#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/parsing/nodes/ExpressionSyntax.hpp>
#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxToken.hpp>
#include <shard/parsing/SyntaxNode.hpp>

#include <memory>

namespace shard
{
	class OperatorSymbol;

	class SHARD_API BinaryExpressionSyntax : public ExpressionSyntax
	{
	public:
		const SyntaxToken OperatorToken;
		std::unique_ptr<ExpressionSyntax> Left = nullptr;
		std::unique_ptr<ExpressionSyntax> Right = nullptr;
		OperatorSymbol* ToOperator = nullptr;

		inline BinaryExpressionSyntax(const SyntaxToken operatorToken, SyntaxNode* parent)
			: ExpressionSyntax(SyntaxKind::BinaryExpression, parent), OperatorToken(operatorToken) {}
		
		inline BinaryExpressionSyntax(const BinaryExpressionSyntax&) = delete;

		inline virtual ~BinaryExpressionSyntax() = default;
	};
}
