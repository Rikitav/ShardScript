#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/syntax/SyntaxToken.h>
#include <shard/syntax/TokenType.h>
#include <shard/parsing/analysis/TextLocation.h>

#include <locale>
#include <string>
#include <deque>

namespace shard
{
	class SHARD_API SourceProvider
	{
	public:
		virtual shard::SyntaxToken Current() = 0;
		virtual shard::SyntaxToken Consume() = 0;
		virtual shard::SyntaxToken Peek(int index = 0) = 0;
		
		virtual bool CanConsume() = 0;
		virtual bool CanPeek() = 0;
	};
}