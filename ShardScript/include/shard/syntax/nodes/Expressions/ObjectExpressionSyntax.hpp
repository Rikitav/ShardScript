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

		std::unique_ptr<ArgumentsListSyntax> ArgumentsList = nullptr;
		std::unique_ptr<TypeSyntax> Type = nullptr;
		
		ConstructorSymbol* CtorSymbol = nullptr;
		TypeSymbol* TypeSymbol = nullptr;

		inline ObjectExpressionSyntax(SyntaxNode *const parent)
			: ExpressionSyntax(SyntaxKind::ObjectExpression, parent) { }

		inline ObjectExpressionSyntax(const ObjectExpressionSyntax&) = delete;

		inline virtual ~ObjectExpressionSyntax() = default;
	};
}
