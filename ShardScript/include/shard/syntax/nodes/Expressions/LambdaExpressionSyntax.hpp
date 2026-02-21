#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/syntax/SyntaxKind.hpp>
#include <shard/syntax/SyntaxToken.hpp>
#include <shard/syntax/SyntaxNode.hpp>

#include <shard/syntax/nodes/ExpressionSyntax.hpp>
#include <shard/syntax/nodes/StatementsBlockSyntax.hpp>
#include <shard/syntax/nodes/ParametersListSyntax.hpp>
#include <shard/syntax/symbols/DelegateTypeSymbol.hpp>

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
