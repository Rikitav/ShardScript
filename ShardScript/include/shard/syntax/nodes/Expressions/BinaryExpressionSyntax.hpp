#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/syntax/nodes/ExpressionSyntax.hpp>
#include <shard/syntax/SyntaxKind.hpp>
#include <shard/syntax/SyntaxToken.hpp>
#include <shard/syntax/SyntaxNode.hpp>

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
