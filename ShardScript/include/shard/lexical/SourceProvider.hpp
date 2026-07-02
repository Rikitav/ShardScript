#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/parsing/SyntaxToken.hpp>
#include <shard/lexical/TokenType.hpp>
#include <shard/analysis/TextLocation.hpp>

#include <locale>
#include <string>
#include <deque>

namespace shard
{
	class SHARD_API SourceProvider
	{
	public:
		virtual ~SourceProvider() = default;

		virtual shard::SyntaxToken Current() = 0;
		virtual shard::SyntaxToken Consume() = 0;
		virtual shard::SyntaxToken Peek(int index = 0) = 0;
		virtual void PutBackToken(shard::SyntaxToken token) = 0;

		virtual bool CanConsume() = 0;
		virtual bool CanPeek() = 0;
	};
}