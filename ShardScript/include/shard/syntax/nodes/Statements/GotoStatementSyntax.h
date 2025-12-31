#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/syntax/SyntaxNode.h>
#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxToken.h>

#include <shard/syntax/nodes/StatementSyntax.h>

#include <shard/syntax/symbols/GotoMarkSymbol.h>

namespace shard
{
	class SHARD_API GotoStatementSyntax : public KeywordStatementSyntax
	{
	public:
		shard::SyntaxToken MarkIdentifierToken;
		shard::GotoMarkSymbol* MarkSymbol = nullptr;

		inline GotoStatementSyntax(const SyntaxNode* parent)
			: KeywordStatementSyntax(SyntaxKind::GotoStatement, parent) { }
		
		inline GotoStatementSyntax(const GotoStatementSyntax& other) = delete;

		inline virtual ~GotoStatementSyntax()
		{

		}
	};
}
