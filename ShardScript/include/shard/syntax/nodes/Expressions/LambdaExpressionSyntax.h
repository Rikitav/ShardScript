#pragma once
#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxToken.h>
#include <shard/syntax/SyntaxNode.h>

#include <shard/syntax/nodes/ExpressionSyntax.h>
#include <shard/syntax/nodes/StatementsBlockSyntax.h>
#include <shard/syntax/nodes/ParametersListSyntax.h>
#include <shard/syntax/symbols/DelegateTypeSymbol.h>

namespace shard::syntax::nodes
{
	class LambdaExpressionSyntax : public ExpressionSyntax
	{
	public:
		SyntaxToken LambdaToken;
		SyntaxToken LambdaOperatorToken;
		ParametersListSyntax* Params = nullptr;
		StatementsBlockSyntax* Body = nullptr;
		shard::syntax::symbols::DelegateTypeSymbol* Symbol = nullptr;

		inline LambdaExpressionSyntax(const SyntaxNode* parent)
			: ExpressionSyntax(SyntaxKind::LambdaExpression, parent) { }

		LambdaExpressionSyntax(const LambdaExpressionSyntax&) = delete;

		inline virtual ~LambdaExpressionSyntax()
		{
			if (Params != nullptr)
				delete Params;
			
			if (Body != nullptr)
				delete Body;
		}
	};
}
