#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/syntax/SyntaxNode.h>
#include <shard/syntax/SyntaxKind.h>

#include <shard/syntax/nodes/BodyDeclarationSyntax.h>
#include <shard/syntax/nodes/StatementSyntax.h>

#include <vector>

namespace shard
{
	class SHARD_API StatementsBlockSyntax : public BodyDeclarationSyntax
	{
	public:
		std::vector<StatementSyntax*> Statements;

		inline StatementsBlockSyntax(SyntaxNode *const parent)
			: BodyDeclarationSyntax(SyntaxKind::StatementsBlock, parent) { }

		inline StatementsBlockSyntax(const StatementsBlockSyntax& other) = delete;

		inline virtual ~StatementsBlockSyntax()
		{
			for (const StatementSyntax* statement : Statements)
				delete statement;
		}
	};
}
