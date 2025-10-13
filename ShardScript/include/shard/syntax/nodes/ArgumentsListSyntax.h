#pragma once
#include <vector>
#include <memory>
#include <shard/syntax/SyntaxNode.h>
#include <shard/syntax/analysis/DiagnosticsContext.h>
#include <shard/syntax/nodes/ExpressionSyntax.h>
#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxToken.h>

namespace shard::syntax::nodes
{
	class ArgumentSyntax : public SyntaxNode
	{
	public:
		shared_ptr<ExpressionSyntax> Expression;

		ArgumentSyntax(shared_ptr<ExpressionSyntax> expr)
			: SyntaxNode(SyntaxKind::Argument), Expression(expr) {}
	};

	class ArgumentsListSyntax : public SyntaxNode
	{
	public:
		SyntaxToken OpenCurlToken;
		SyntaxToken CloseCurlToken;
		vector<shared_ptr<ArgumentSyntax>> Arguments;

		ArgumentsListSyntax() : SyntaxNode(SyntaxKind::ArgumentsList) {}
	};
}