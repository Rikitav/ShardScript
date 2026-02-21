#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <string>

namespace shard
{
	struct SHARD_API TextLocation
	{
	public:
		const std::wstring FileName;
		const int Line;
		const int Offset;
		const int Length;

		inline TextLocation()
			: FileName(L""), Line(0), Offset(0), Length(0) { }

		inline TextLocation(std::wstring filename, int line, int offset, int length)
			: FileName(filename), Line(line), Offset(offset), Length(length) {}

		inline TextLocation(const TextLocation& other)
			: FileName(other.FileName), Line(other.Line), Offset(other.Offset), Length(other.Length) { }
	};
}
