#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxToken.hpp>
#include <shard/parsing/SyntaxNode.hpp>

#include <shard/parsing/nodes/TypeSyntax.hpp>
#include <shard/parsing/nodes/ArgumentsListSyntax.hpp>
#include <shard/parsing/nodes/ExpressionSyntax.hpp>

#include <shard/semantic/symbols/TypeSymbol.hpp>
#include <shard/semantic/symbols/ConstructorSymbol.hpp>

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
		TypeSymbol* Symbol = nullptr;

		inline ObjectExpressionSyntax(SyntaxNode* parent)
			: ExpressionSyntax(SyntaxKind::ObjectExpression, parent) { }

		inline ObjectExpressionSyntax(const ObjectExpressionSyntax&) = delete;

		inline virtual ~ObjectExpressionSyntax() = default;
	};
}
