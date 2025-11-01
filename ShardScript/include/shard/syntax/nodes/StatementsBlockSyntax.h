#pragma once
#include <shard/syntax/SyntaxNode.h>
#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/nodes/BodyDeclarationSyntax.h>
#include <shard/syntax/nodes/StatementSyntax.h>
#include <vector>

namespace shard::syntax::nodes
{
	class StatementsBlockSyntax : public BodyDeclarationSyntax
	{
	public:
		std::vector<StatementSyntax*> Statements;

		inline StatementsBlockSyntax(const SyntaxNode* parent)
			: BodyDeclarationSyntax(SyntaxKind::StatementsBlock, parent) { }

		inline StatementsBlockSyntax(const StatementsBlockSyntax& other)
			: BodyDeclarationSyntax(other), Statements(other.Statements) { }

		inline virtual ~StatementsBlockSyntax()
		{
			for (const StatementSyntax* statement : Statements)
			{
				statement->~StatementSyntax();
				delete statement;
			}

			Statements.~vector();
		}
	};
}
