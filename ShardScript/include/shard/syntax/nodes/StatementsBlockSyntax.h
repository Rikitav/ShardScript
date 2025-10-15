#pragma once
#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/nodes/BodyDeclarationSyntax.h>
#include <shard/syntax/nodes/StatementSyntax.h>
#include <memory>
#include <vector>

namespace shard::syntax::nodes
{
	class StatementsBlockSyntax : public BodyDeclarationSyntax
	{
	public:
		vector<shared_ptr<StatementSyntax>> Statements;

		StatementsBlockSyntax() : BodyDeclarationSyntax(SyntaxKind::StatementsBlock)
		{

		}
	};
}
