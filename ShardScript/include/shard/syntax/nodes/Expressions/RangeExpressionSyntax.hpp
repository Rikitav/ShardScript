#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/syntax/nodes/ExpressionSyntax.hpp>
#include <shard/syntax/SyntaxKind.hpp>
#include <shard/syntax/SyntaxToken.hpp>
#include <shard/syntax/SyntaxNode.hpp>

namespace shard
{
	class SHARD_API RangeExpressionSyntax : public ExpressionSyntax
	{
	public:
		SyntaxToken OperatorToken;
		
		std::unique_ptr<ExpressionSyntax> Left = nullptr;
		std::unique_ptr<ExpressionSyntax> Right = nullptr;

		bool IsInclusive = false;

		inline RangeExpressionSyntax(SyntaxNode* parent)
			: ExpressionSyntax(SyntaxKind::RangeExpression, parent) { }

		inline RangeExpressionSyntax(const RangeExpressionSyntax& other) = delete;

		inline virtual ~RangeExpressionSyntax() = default;
	};
}
