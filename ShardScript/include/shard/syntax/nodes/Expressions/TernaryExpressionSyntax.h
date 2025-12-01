#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/syntax/nodes/ExpressionSyntax.h>
#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxToken.h>
#include <shard/syntax/SyntaxNode.h>

namespace shard::syntax::nodes
{
	class SHARD_API TernaryExpressionSyntax : public ExpressionSyntax
	{
	public:
		SyntaxToken QuestionToken;
		SyntaxToken ColonToken;
		ExpressionSyntax* Condition = nullptr;
		ExpressionSyntax* Left = nullptr;
		ExpressionSyntax* Right = nullptr;

		inline TernaryExpressionSyntax(const SyntaxNode* parent)
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
