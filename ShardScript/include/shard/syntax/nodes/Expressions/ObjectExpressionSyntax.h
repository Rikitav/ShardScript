#pragma once
#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxToken.h>
#include <shard/syntax/SyntaxNode.h>

#include <shard/syntax/nodes/TypeSyntax.h>
#include <shard/syntax/nodes/ArgumentsListSyntax.h>
#include <shard/syntax/nodes/ExpressionSyntax.h>

#include <shard/syntax/symbols/TypeSymbol.h>

namespace shard::syntax::nodes
{
	class ObjectExpressionSyntax : public ExpressionSyntax
	{
	public:
		SyntaxToken NewToken;
		SyntaxToken IdentifierToken;

		TypeSyntax* Type = nullptr;
		ArgumentsListSyntax* Arguments = nullptr;
		shard::syntax::symbols::TypeSymbol* Symbol = nullptr;

		inline ObjectExpressionSyntax(const SyntaxNode* parent)
			: ExpressionSyntax(SyntaxKind::ObjectExpression, parent) { }

		inline ObjectExpressionSyntax(const ObjectExpressionSyntax& other)
			: ExpressionSyntax(other), NewToken(other.NewToken), IdentifierToken(other.IdentifierToken), Type(other.Type), Arguments(other.Arguments), Symbol(other.Symbol) { }

		inline virtual ~ObjectExpressionSyntax()
		{
			if (Type != nullptr)
			{
				Type->~TypeSyntax();
				delete Type;
			}

			if (Arguments != nullptr)
			{
				Arguments->~ArgumentsListSyntax();
				delete Arguments;
			}
		}
	};
}
