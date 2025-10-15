#pragma once
#include <shard/syntax/TokenType.h>
#include <shard/syntax/analysis/TextLocation.h>
#include <string>

using namespace std;
using namespace shard::syntax::analysis;

namespace shard::syntax
{
	struct SyntaxToken
	{
	private:
		inline static int counter = 0;

	public:
		string Word;
		TokenType Type;
		TextLocation Location;
		bool IsMissing;
		int Index;

		SyntaxToken()
			: Word(), Type(TokenType::Unknown), IsMissing(true), Index(-1) {}

		SyntaxToken(TokenType type, string word, TextLocation location, bool isMissing = false)
			: Word(word), Type(type), Location(location), IsMissing(isMissing), Index(counter++) {}
	};
}
