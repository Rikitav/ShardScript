#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/syntax/SyntaxKind.hpp>
#include <shard/syntax/SyntaxToken.hpp>
#include <shard/syntax/SyntaxNode.hpp>

#include <shard/syntax/nodes/TypeSyntax.hpp>
#include <shard/syntax/nodes/ArgumentsListSyntax.hpp>
#include <shard/syntax/nodes/ExpressionSyntax.hpp>

#include <shard/syntax/symbols/TypeSymbol.hpp>
#include <shard/syntax/symbols/ConstructorSymbol.hpp>

namespace shard
{
	class SHARD_API ObjectExpressionSyntax : public ExpressionSyntax
	{
	public:
		SyntaxToken NewToken;
		SyntaxToken IdentifierToken;

		TypeSyntax* Type = nullptr;
		ArgumentsListSyntax* ArgumentsList = nullptr;
		shard::TypeSymbol* TypeSymbol = nullptr;
		shard::ConstructorSymbol* CtorSymbol = nullptr;

		inline ObjectExpressionSyntax(SyntaxNode *const parent)
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
