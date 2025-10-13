#include <shard/parsing/SourceReader.h>
#include <shard/syntax/analysis/TextLocation.h>
#include <sstream>
#include <string>

using namespace std;

namespace shard::parsing
{
	class StringStreamReader : public SourceReader
	{
		istringstream stringStream;

	public:
		StringStreamReader(const string& source)
		{
			stringStream = istringstream(source);
		}

	protected:
		TextLocation GetLocation(string word)
		{
			return TextLocation("<STRING>", Line, Offset, word.length());
		}

		bool ReadNext()
		{
			if (!stringStream.get(Symbol))
				return false;

			Offset++;
			return true;
		}

		bool PeekNext()
		{
			PeekSymbol = stringStream.peek();
			return PeekSymbol != -1;
		}
	};
}
