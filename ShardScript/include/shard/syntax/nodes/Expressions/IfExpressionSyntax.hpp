#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/syntax/nodes/ExpressionSyntax.hpp>
#include <shard/syntax/SyntaxKind.hpp>
#include <shard/syntax/SyntaxToken.hpp>
#include <shard/syntax/SyntaxNode.hpp>

namespace shard
{
	class SHARD_API IfExpressionSyntax : public ExpressionSyntax
	{
	public:
		SyntaxToken IfKeywordToken;
		SyntaxToken OpenBraceToken;
		SyntaxToken CloseBraceToken;
		SyntaxToken ElseKeywordToken;

		ExpressionSyntax* Condition = nullptr;
		ExpressionSyntax* ThenExpression = nullptr;
		ExpressionSyntax* ElseExpression = nullptr;

		inline IfExpressionSyntax(SyntaxNode* const parent)
			: ExpressionSyntax(SyntaxKind::IfExpression, parent) { }

		inline IfExpressionSyntax(const IfExpressionSyntax&) = delete;

		inline virtual ~IfExpressionSyntax()
		{
			if (Condition != nullptr)
				delete Condition;

			if (ThenExpression != nullptr)
				delete ThenExpression;
			
			if (ElseExpression != nullptr)
				delete ElseExpression;
		}
	};
}
