#pragma once
#include <shard/Definitions.hpp>

#include <cstdint>
#include <string>

namespace shard
{
	struct SyntaxToken;

	struct SHARD_API TextLocation
	{
		std::wstring m_fileName;
		std::int32_t m_line;
		std::int32_t m_offset;
		std::int32_t m_length;

	public:
		TextLocation();
		TextLocation(const SyntaxToken& left, const SyntaxToken& right);
		TextLocation(const std::wstring& filename, std::int32_t line, std::int32_t offset, std::int32_t length);

		inline std::int32_t get_line() const { return m_line; }
		inline std::int32_t get_offset() const { return m_offset; }
		inline std::int32_t get_length() const { return m_length; }
		inline string_t get_filename() const { return m_fileName.c_str(); }
	};
}
