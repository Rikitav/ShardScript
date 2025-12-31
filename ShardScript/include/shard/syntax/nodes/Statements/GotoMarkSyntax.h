#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/syntax/SyntaxNode.h>
#include <shard/syntax/SyntaxKind.h>

#include <shard/syntax/nodes/StatementSyntax.h>

namespace shard
{
	class SHARD_API GotoMarkSyntax : public StatementSyntax
	{
	public:
		const shard::SyntaxToken IdentifierToken;
		const shard::SyntaxToken ColonToken;

		inline GotoMarkSyntax(const shard::SyntaxToken identifierToken, const shard::SyntaxToken colonToken, const SyntaxNode* parent)
			: StatementSyntax(SyntaxKind::GotoMarkStatement, parent), IdentifierToken(identifierToken), ColonToken(colonToken) { }
		
		inline GotoMarkSyntax(const GotoMarkSyntax& other) = delete;

		inline virtual ~GotoMarkSyntax() { }
	};
}
