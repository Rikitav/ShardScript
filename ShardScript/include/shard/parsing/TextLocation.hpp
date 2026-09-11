#pragma once
#include <shard/Definitions.hpp>

#include <string>

namespace shard
{
	struct SHARD_API TextLocation
	{
		std::wstring m_fileName;
		int m_line;
		int m_offset;
		int m_length;

	public:
		inline TextLocation() :
			m_fileName(L""),
			m_line(0),
			m_offset(0),
			m_length(0)
		{ }

		inline TextLocation(std::wstring filename, int line, int offset, int length) :
			m_fileName(filename),
			m_line(line),
			m_offset(offset),
			m_length(length)
		{ }

		inline int get_line() const { return m_line; }
		inline int get_offset() const { return m_offset; }
		inline int get_length() const { return m_length; }
		inline const wchar_t* get_filename() const { return m_fileName.c_str(); }
	};
}
