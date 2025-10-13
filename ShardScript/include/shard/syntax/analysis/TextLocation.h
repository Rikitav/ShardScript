#pragma once
#include <string>

using namespace std;

namespace shard::syntax::analysis
{
	struct TextLocation
	{
	public:
		string FileName;
		int Line;
		int Offset;
		int Length;

		TextLocation()
			: FileName(""), Line(0), Offset(0), Length(0) { }

		TextLocation(string filename, int line, int offset, int length)
			: FileName(filename), Line(line), Offset(offset), Length(length) {}
	};
}
