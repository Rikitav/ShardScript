#pragma once
#include <string>

namespace shard::parsing::analysis
{
	struct TextLocation
	{
	public:
		const std::wstring FileName;
		const int Line;
		const int Offset;
		const int Length;

		TextLocation()
			: FileName(L""), Line(0), Offset(0), Length(0) { }

		TextLocation(std::wstring filename, int line, int offset, int length)
			: FileName(filename), Line(line), Offset(offset), Length(length) {}

		TextLocation(const TextLocation& other)
			: FileName(other.FileName), Line(other.Line), Offset(other.Offset), Length(other.Length) { }
	};
}
