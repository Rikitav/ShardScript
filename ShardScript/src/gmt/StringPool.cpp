#include <gmt/StringPool.hpp>

#include <algorithm>
#include <cassert>
#include <iterator>
#include <limits>
#include <utility>
#include <vector>

using namespace gmt;

namespace
{
	class StringBlock
	{
		static constexpr wchar_t null_char = L'\0';

		std::unique_ptr<wchar_t[]> m_memory;
		std::size_t m_size;
		std::size_t m_free_space;

	public:
		using string_type = std::wstring_view;

		StringBlock(
			std::size_t element_count
		) :
			m_memory(new wchar_t[element_count]),
			m_size(element_count),
			m_free_space(element_count)
		{ }

		[[nodiscard]] string_type add_string(string_type string)
		{
			if (!can_take_string_of_length(string.length()))
			{
				assert(false && "StringBlock doesn't have enough capacity to store the string");
				return &null_char;
			}

			wchar_t* destination = m_memory.get() + get_used_space();
			std::copy(string.begin(), string.end(), destination);
			destination[string.length()] = null_char;

			m_free_space -= get_space_required_to_store_string_of_length(string.length());
			return { destination, string.length() };
		}

		[[nodiscard]] bool can_take_string_of_length(std::size_t length) const noexcept
		{
			return m_free_space >= get_space_required_to_store_string_of_length(length) && is_string_length_valid(length);
		}

		[[nodiscard]] static constexpr std::size_t get_space_required_to_store_string_of_length(std::size_t length) noexcept
		{
			return is_string_length_valid(length) ? length + 1 : length;
		}

		[[nodiscard]] std::size_t get_free_space() const noexcept
		{
			return m_free_space;
		}

		friend void swap(StringBlock& a, StringBlock& b) noexcept
		{
			a.m_memory.swap(b.m_memory);
			std::swap(a.m_size, b.m_size);
			std::swap(a.m_free_space, b.m_free_space);
		}

	private:
		[[nodiscard]] std::size_t get_used_space() const noexcept
		{
			return m_size - m_free_space;
		}

		[[nodiscard]] static constexpr bool is_string_length_valid(std::size_t length) noexcept
		{
			return length != (std::numeric_limits<std::size_t>::max)();
		}
	};

	template<typename BlockIterator>
	class StringBlockEditor
	{
	public:
		StringBlockEditor(BlockIterator first_block, BlockIterator edited_block) :
			m_first_block(first_block),
			m_edited_block(edited_block)
		{ }

		[[nodiscard]] std::wstring_view add_string(std::wstring_view string)
		{
			return m_edited_block->add_string(string);
		}

		~StringBlockEditor()
		{
			if (should_reorder_blocks_after_adding_string_to_block())
				reorder_blocks_after_adding_string_to_block();
		}

	private:
		[[nodiscard]] bool should_reorder_blocks_after_adding_string_to_block() const
		{
			return m_edited_block != m_first_block && m_edited_block->get_free_space() < std::prev(m_edited_block)->get_free_space();
		}

		void reorder_blocks_after_adding_string_to_block() const
		{
			move_edited_block_in_place_of(get_first_block_with_more_free_space_than_edited_block());
		}

		[[nodiscard]] BlockIterator get_first_block_with_more_free_space_than_edited_block() const
		{
			return std::upper_bound(m_first_block, m_edited_block, m_edited_block->get_free_space(),
				[](const auto free_space, const auto& block) { return free_space < block.get_free_space(); });
		}

		void move_edited_block_in_place_of(BlockIterator block) const
		{
			if (block == m_edited_block)
				return;

			if (block->get_free_space() == std::prev(m_edited_block)->get_free_space())
			{
				std::iter_swap(block, m_edited_block);
			}
			else
			{
				while (block != m_edited_block)
				{
					std::iter_swap(block, m_edited_block);
					++block;
				}
			}
		}

		BlockIterator m_first_block;
		BlockIterator m_edited_block;
	};
}

class StringPool::Impl
{
	using BlockList = std::vector<StringBlock>;
	using BlockIterator = BlockList::iterator;

	BlockList m_blocks;
	std::size_t m_standard_block_capacity;

public:
	explicit Impl(std::size_t standard_block_capacity) :
		m_blocks(),
		m_standard_block_capacity(standard_block_capacity)
	{ }

	[[nodiscard]] std::wstring_view add(std::wstring_view string)
	{
		return get_block_capable_of_storing_string_of_length(string.length()).add_string(string);
	}

	[[nodiscard]] std::size_t get_block_count() const noexcept
	{
		return m_blocks.size();
	}

	[[nodiscard]] std::size_t get_standard_block_capacity() const noexcept
	{
		return m_standard_block_capacity;
	}

	void set_standard_block_capacity(std::size_t new_standard_block_capacity) noexcept
	{
		m_standard_block_capacity = new_standard_block_capacity;
	}

private:
	[[nodiscard]] StringBlockEditor<BlockIterator> get_block_capable_of_storing_string_of_length(std::size_t length)
	{
		const BlockIterator block = find_or_create_block_capable_of_storing_string_of_length(length);
		return { m_blocks.begin(), block };
	}

	[[nodiscard]] BlockIterator find_or_create_block_capable_of_storing_string_of_length(std::size_t length)
	{
		if (const BlockIterator block = find_block_capable_of_storing_string_of_length(length); block != m_blocks.end())
			return block;

		return create_block_capable_of_storing_string_of_length(length);
	}

	[[nodiscard]] BlockIterator find_block_capable_of_storing_string_of_length(std::size_t length)
	{
		return std::partition_point(get_first_block_maybe_capable_of_storing_string_of_length(length), m_blocks.end(),
			[length](const auto& block) { return !block.can_take_string_of_length(length); });
	}

	[[nodiscard]] BlockIterator create_block_capable_of_storing_string_of_length(std::size_t length)
	{
		m_blocks.emplace_back((std::max)(m_standard_block_capacity, StringBlock::get_space_required_to_store_string_of_length(length)));
		return std::prev(m_blocks.end());
	}

	[[nodiscard]] BlockIterator get_first_block_maybe_capable_of_storing_string_of_length(std::size_t length)
	{
		const BlockIterator begin = m_blocks.begin(), end = m_blocks.end();
		if (std::distance(begin, end) > 2 && !std::prev(end, 2)->can_take_string_of_length(length))
			return std::prev(end);

		return begin;
	}
};

StringPool::StringPool() :
	m_impl(std::make_unique<Impl>(8192))
{ }

StringPool::StringPool(
	std::size_t standard_block_capacity
) :
	m_impl(std::make_unique<Impl>(standard_block_capacity))
{ }

StringPool::~StringPool() = default;

std::wstring_view StringPool::add(std::wstring_view string)
{
	return m_impl->add(string);
}

std::wstring_view StringPool::intern(std::wstring_view string)
{
	return m_impl->add(string);
}

std::size_t StringPool::get_block_count() const noexcept
{
	return m_impl->get_block_count();
}

std::size_t StringPool::get_standard_block_capacity() const noexcept
{
	return m_impl->get_standard_block_capacity();
}

void StringPool::set_standard_block_capacity(std::size_t new_standard_block_capacity) noexcept
{
	m_impl->set_standard_block_capacity(new_standard_block_capacity);
}
