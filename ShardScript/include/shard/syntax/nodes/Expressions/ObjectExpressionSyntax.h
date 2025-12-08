#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxToken.h>
#include <shard/syntax/SyntaxNode.h>

#include <shard/syntax/nodes/TypeSyntax.h>
#include <shard/syntax/nodes/ArgumentsListSyntax.h>
#include <shard/syntax/nodes/ExpressionSyntax.h>

#include <shard/syntax/symbols/TypeSymbol.h>
#include <shard/syntax/symbols/ConstructorSymbol.h>

namespace shard::syntax::nodes
{
	class SHARD_API ObjectExpressionSyntax : public ExpressionSyntax
	{
	public:
		SyntaxToken NewToken;
		SyntaxToken IdentifierToken;

		TypeSyntax* Type = nullptr;
		ArgumentsListSyntax* ArgumentsList = nullptr;
		shard::syntax::symbols::TypeSymbol* TypeSymbol = nullptr;
		shard::syntax::symbols::ConstructorSymbol* CtorSymbol = nullptr;

		inline ObjectExpressionSyntax(const SyntaxNode* parent)
			: ExpressionSyntax(SyntaxKind::ObjectExpression, parent) { }

		inline ObjectExpressionSyntax(const ObjectExpressionSyntax&) = delete;

		inline virtual ~ObjectExpressionSyntax()
		{
			if (Type != nullptr)
				delete Type;

			if (ArgumentsList != nullptr)
				delete ArgumentsList;
		}
	};
}
