#include <shard/parsing/SourceReader.h>
#include <vector>

using namespace std;

namespace shard::parsing
{
	class SequenceSourceReader : public SourceReader
	{
		vector<SyntaxToken> Sequence;
		size_t VectorIndex;

	public:
		SequenceSourceReader() : Sequence(), VectorIndex(0)
		{

		}

		void Populate(vector<SyntaxToken> fromVector)
		{
			for (const SyntaxToken& fromToken : fromVector)
				Sequence.push_back(fromToken);
		}

		void Clear()
		{
			VectorIndex = 0;
			Sequence.clear();
		}

		void SetSequence(vector<SyntaxToken> setVector)
		{
			Clear();
			Sequence = setVector;
		}

		void SetIndex(size_t newIndex)
		{
			VectorIndex = newIndex;
		}

		SyntaxToken Current()
		{
			if (VectorIndex >= Sequence.size())
				return SyntaxToken(TokenType::EndOfFile, "", TextLocation());

			return Sequence[VectorIndex];
		}

		SyntaxToken Consume()
		{
			VectorIndex += 1;
			return Current();
		}

		SyntaxToken Peek()
		{
			return Sequence[VectorIndex + 1];
		}

		bool CanConsume()
		{
			return VectorIndex < Sequence.size();
		}

		bool CanPeek()
		{
			return VectorIndex < Sequence.size() - 1;
		}

	protected:
		TextLocation GetLocation(string word)
		{
			return TextLocation();
		}

		bool ReadNext()
		{
			return false;
		}

		bool PeekNext()
		{
			return false;
		}
	};
}
