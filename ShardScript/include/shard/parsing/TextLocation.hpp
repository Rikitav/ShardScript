#pragma once
#include <shard/Definitions.hpp>

#include <cstdint>
#include <string_view>

namespace shard
{
	struct SyntaxToken;

	struct SHARD_API TextLocation
	{
		std::wstring_view m_fileName;
		std::int32_t m_line;
		std::int32_t m_offset;
		std::int32_t m_length;

	public:
		TextLocation();
		TextLocation(const SyntaxToken& left, const SyntaxToken& right);
		TextLocation(const TextLocation& left, const TextLocation& right);
		TextLocation(const TextLocation& left, const SyntaxToken& right);
		TextLocation(const SyntaxToken& left, const TextLocation& right);
		TextLocation(const std::wstring_view filename, std::int32_t line, std::int32_t offset, std::int32_t length);

		inline std::int32_t get_line() const { return m_line; }
		inline std::int32_t get_offset() const { return m_offset; }
		inline std::int32_t get_length() const { return m_length; }
		inline std::wstring_view get_filename() const { return m_fileName; }
	};
}
