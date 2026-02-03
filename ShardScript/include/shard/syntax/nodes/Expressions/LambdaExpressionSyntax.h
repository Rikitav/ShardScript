#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxToken.h>
#include <shard/syntax/SyntaxNode.h>

#include <shard/syntax/nodes/ExpressionSyntax.h>
#include <shard/syntax/nodes/StatementsBlockSyntax.h>
#include <shard/syntax/nodes/ParametersListSyntax.h>
#include <shard/syntax/symbols/DelegateTypeSymbol.h>

namespace shard
{
	class SHARD_API LambdaExpressionSyntax : public ExpressionSyntax
	{
	public:
		SyntaxToken LambdaToken;
		SyntaxToken LambdaOperatorToken;
		ParametersListSyntax* Params = nullptr;
		StatementsBlockSyntax* Body = nullptr;
		shard::DelegateTypeSymbol* Symbol = nullptr;

		inline LambdaExpressionSyntax(SyntaxNode *const parent)
			: ExpressionSyntax(SyntaxKind::LambdaExpression, parent) { }

		inline LambdaExpressionSyntax(const LambdaExpressionSyntax&) = delete;

		inline virtual ~LambdaExpressionSyntax()
		{
			if (Params != nullptr)
				delete Params;
			
			if (Body != nullptr)
				delete Body;
		}
	};
}
