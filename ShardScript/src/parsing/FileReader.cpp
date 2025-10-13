#include <shard/parsing/SourceReader.h>
#include <fstream>
#include <stdexcept>
#include <string>

using namespace std;

namespace shard::parsing
{
	class FileReader : public SourceReader
	{
		string Filename;
		fstream InputStream;

	public:
		FileReader(const string fileName)
		{
			Filename = fileName;

			InputStream = fstream(fileName, ios::in);
			if (!InputStream)
				throw new runtime_error("Cannot open file");
		}

	protected:
		TextLocation GetLocation(string word)
		{
			return TextLocation(Filename, Line, Offset, word.length());
		}

		bool ReadNext()
		{
			if (!InputStream.get(Symbol))
				return false;

			Offset++;
			return true;
		}

		bool PeekNext()
		{
			PeekSymbol = InputStream.peek();
			return PeekSymbol != -1;
		}
	};
}