#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxToken.hpp>
#include <shard/parsing/TokenType.hpp>
#include <shard/parsing/TextLocation.hpp>

namespace shard
{
	class SHARD_API SourceProvider
	{
	public:
		virtual ~SourceProvider() = default;

		virtual SyntaxToken current() = 0;
		virtual SyntaxToken consume() = 0;
		virtual SyntaxToken peek(int index = 0) = 0;
		virtual void put_back(SyntaxToken token) = 0;

		virtual bool can_consume() = 0;
		virtual bool can_peek() = 0;
	};
}