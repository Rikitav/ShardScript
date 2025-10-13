#pragma once
#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/nodes/BlockDeclarationSyntax.h>
#include <shard/syntax/nodes/StatementSyntax.h>
#include <vector>
#include <memory>

namespace shard::syntax::nodes
{
	class MethodBodySyntax : public BlockDeclarationSyntax
	{
	public:
		vector<shared_ptr<StatementSyntax>> Statements;
		//MethodDeclarationSyntax* Method;

		MethodBodySyntax() : BlockDeclarationSyntax(SyntaxKind::MethodBody)
		{

		}
	};
}
