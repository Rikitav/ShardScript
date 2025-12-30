#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/syntax/TokenType.h>
#include <shard/parsing/analysis/TextLocation.h>

#include <string>
#include <utility>

namespace shard
{
	struct SHARD_API SyntaxToken
	{
	private:
		inline static int counter = 0;

	public:
		const int Index;
		const bool IsMissing;
		const TokenType Type;
		const std::wstring Word;
		const shard::TextLocation Location;

		inline SyntaxToken()
			: Index(-1), IsMissing(true), Type(TokenType::Unknown), Word() { }

		inline SyntaxToken(const TokenType type, std::wstring word, const shard::TextLocation& location, const bool isMissing = false)
			: Index(counter++), IsMissing(isMissing), Type(type), Word(std::move(word)), Location(location) { }

		inline SyntaxToken(const SyntaxToken& other)
			: Index(other.Index), IsMissing(other.IsMissing), Type(other.Type), Word(other.Word), Location(other.Location) { }

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
