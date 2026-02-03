#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/syntax/nodes/ExpressionSyntax.h>
#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxToken.h>
#include <shard/syntax/SyntaxNode.h>

namespace shard
{
	class SHARD_API BinaryExpressionSyntax : public ExpressionSyntax
	{
	public:
		const SyntaxToken OperatorToken;
		ExpressionSyntax* Left = nullptr;
		ExpressionSyntax* Right = nullptr;

		inline BinaryExpressionSyntax(const SyntaxToken operatorToken, SyntaxNode *const parent)
			: ExpressionSyntax(SyntaxKind::BinaryExpression, parent), OperatorToken(operatorToken) {}
		
		inline BinaryExpressionSyntax(const BinaryExpressionSyntax&) = delete;

		inline virtual ~BinaryExpressionSyntax()
		{
			delete Left;
			delete Right;
		}
	};
}
