#pragma once
#include <string>

namespace shard::syntax::analysis
{
	struct TextLocation
	{
	public:
		std::string FileName;
		int Line;
		int Offset;
		int Length;

		TextLocation()
			: FileName(""), Line(0), Offset(0), Length(0) { }

		TextLocation(std::string filename, int line, int offset, int length)
			: FileName(filename), Line(line), Offset(offset), Length(length) {}
	};
}
