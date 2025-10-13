#pragma once
#include <shard/syntax/SyntaxNode.h>
#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/analysis/DiagnosticsContext.h>

#include <stdexcept>
#include <string>

namespace shard::syntax::nodes
{
	class ExpressionSyntax : public SyntaxNode
	{
	public:
		ExpressionSyntax(SyntaxKind kind) : SyntaxNode(kind)
		{
			if (kind <= SyntaxKind::Expression || kind > SyntaxKind::InvokationExpression)
				throw runtime_error("Invalid SyntaxKind value range of ExpressionSyntax (" + to_string((int)kind) + ")");
		}
	};
}
