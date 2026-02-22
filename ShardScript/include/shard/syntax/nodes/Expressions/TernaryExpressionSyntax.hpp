#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/syntax/nodes/ExpressionSyntax.hpp>
#include <shard/syntax/SyntaxKind.hpp>
#include <shard/syntax/SyntaxToken.hpp>
#include <shard/syntax/SyntaxNode.hpp>

namespace shard
{
	class SHARD_API TernaryExpressionSyntax : public ExpressionSyntax
	{
	public:
		SyntaxToken QuestionToken;
		SyntaxToken ColonToken;
		ExpressionSyntax* Condition = nullptr;
		ExpressionSyntax* Left = nullptr;
		ExpressionSyntax* Right = nullptr;

		inline TernaryExpressionSyntax(SyntaxNode *const parent)
			: ExpressionSyntax(SyntaxKind::TernaryExpression, parent) { }

		inline TernaryExpressionSyntax(const TernaryExpressionSyntax&) = delete;

		inline virtual ~TernaryExpressionSyntax()
		{
			if (Condition != nullptr)
				delete Condition;
			
			if (Left != nullptr)
				delete Left;

			if (Right != nullptr)
				delete Right;
		}
	};
}
