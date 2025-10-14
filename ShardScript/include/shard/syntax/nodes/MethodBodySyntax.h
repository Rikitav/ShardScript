#pragma once
#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/nodes/BodyDeclarationSyntax.h>
#include <shard/syntax/nodes/StatementSyntax.h>
#include <vector>
#include <memory>

namespace shard::syntax::nodes
{
	class MethodBodySyntax : public BodyDeclarationSyntax
	{
	public:
		vector<shared_ptr<StatementSyntax>> Statements;
		//MethodDeclarationSyntax* Method;

		MethodBodySyntax() : BodyDeclarationSyntax(SyntaxKind::MethodBody)
		{

		}
	};
}
