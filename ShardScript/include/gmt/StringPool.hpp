/*
StringPool - A performant and memory efficient storage for immutable wide strings.

License: MIT 2021-2022 Daniel Krupiński
Modified from: https://github.com/danielkrupinski/StringPool/blob/master/StringPool.h
*/

#pragma once
#include <shard/Definitions.hpp>

#include <cstddef>
#include <memory>
#include <string_view>

namespace gmt
{
	class SHARD_API StringPool
	{
	public:
		StringPool();
		StringPool(std::size_t standard_block_capacity);
		~StringPool();

		StringPool(const StringPool&) = delete;
		StringPool& operator=(const StringPool&) = delete;

		StringPool(StringPool&& other) noexcept;
		StringPool& operator=(StringPool&& other) noexcept;

		[[nodiscard]] std::wstring_view add(std::wstring_view string);
		[[nodiscard]] std::wstring_view intern(std::wstring_view text);

		[[nodiscard]] std::size_t get_block_count() const noexcept;
		[[nodiscard]] std::size_t get_standard_block_capacity() const noexcept;
		void set_standard_block_capacity(std::size_t new_standard_block_capacity) noexcept;

	private:
		class Impl;

		std::unique_ptr<Impl> m_impl;
	};

	inline static StringPool GlobalPool;
}
