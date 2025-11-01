#pragma once
#include <shard/syntax/TokenType.h>
#include <shard/parsing/analysis/TextLocation.h>
#include <string>

namespace shard::syntax
{
	struct SyntaxToken
	{
	private:
		inline static int counter = 0;

	public:
		const int Index;
		const bool IsMissing;
		const TokenType Type;
		const std::wstring Word;
		const shard::parsing::analysis::TextLocation Location;

		inline SyntaxToken()
			: Index(-1), Word(), Type(TokenType::Unknown), IsMissing(true) { }

		inline SyntaxToken(TokenType type, std::wstring word, shard::parsing::analysis::TextLocation location, bool isMissing = false)
			: Index(counter++), IsMissing(isMissing), Type(type), Word(word), Location(location) { }

		inline SyntaxToken(const SyntaxToken& other)
			: Index(other.counter), IsMissing(other.IsMissing), Type(other.Type), Word(other.Word), Location(other.Location) { }

		inline SyntaxToken& operator=(const SyntaxToken& other)
		{
			if (this != &other)
			{
				this->~SyntaxToken();
				new (this) SyntaxToken(other);
			}

			return *this;
		}
	};
}
