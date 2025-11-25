#pragma once
#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxToken.h>
#include <shard/syntax/SyntaxNode.h>

#include <shard/syntax/nodes/TypeSyntax.h>
#include <shard/syntax/nodes/ArgumentsListSyntax.h>
#include <shard/syntax/nodes/ExpressionSyntax.h>

#include <shard/syntax/symbols/TypeSymbol.h>
#include <shard/syntax/symbols/MethodSymbol.h>

namespace shard::syntax::nodes
{
	class ObjectExpressionSyntax : public ExpressionSyntax
	{
	public:
		SyntaxToken NewToken;
		SyntaxToken IdentifierToken;

		TypeSyntax* Type = nullptr;
		ArgumentsListSyntax* ArgumentsList = nullptr;
		shard::syntax::symbols::TypeSymbol* TypeSymbol = nullptr;
		shard::syntax::symbols::MethodSymbol* CtorSymbol = nullptr;

		inline ObjectExpressionSyntax(const SyntaxNode* parent)
			: ExpressionSyntax(SyntaxKind::ObjectExpression, parent) { }

		inline ObjectExpressionSyntax(const ObjectExpressionSyntax& other)
			: ExpressionSyntax(other), NewToken(other.NewToken), IdentifierToken(other.IdentifierToken), Type(other.Type), ArgumentsList(other.ArgumentsList), TypeSymbol(other.TypeSymbol), CtorSymbol(other.CtorSymbol) { }

		inline virtual ~ObjectExpressionSyntax()
		{
			if (Type != nullptr)
			{
				Type->~TypeSyntax();
				delete Type;
			}

			if (ArgumentsList != nullptr)
			{
				ArgumentsList->~ArgumentsListSyntax();
				delete ArgumentsList;
			}
		}
	};
}
