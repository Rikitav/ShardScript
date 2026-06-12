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
		ExpressionSyntax* Left = nullptr;
		ExpressionSyntax* Right = nullptr;
		bool IsInclusive = false;

		inline RangeExpressionSyntax(SyntaxNode *const parent)
			: ExpressionSyntax(SyntaxKind::RangeExpression, parent) { }

		inline RangeExpressionSyntax(const RangeExpressionSyntax& other) = delete;

		inline virtual ~RangeExpressionSyntax()
		{
			if (Left != nullptr)
			{
				Left->~ExpressionSyntax();
				delete Left;
			}

			if (Right != nullptr)
			{
				Right->~ExpressionSyntax();
				delete Right;
			}
		}
	};
}
