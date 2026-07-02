#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/nodes/ExpressionSyntax.hpp>
#include <shard/parsing/nodes/TypeSyntax.hpp>

#include <memory>

namespace shard
{
	class SHARD_API TypeExpressionSyntax : public ExpressionSyntax
	{
	public:
		std::unique_ptr<TypeSyntax> Type;

		inline TypeExpressionSyntax(std::unique_ptr<TypeSyntax>&& type, SyntaxNode* parent)
			: ExpressionSyntax(SyntaxKind::TypeExpression, parent), Type(std::move(type)) { }

		inline TypeExpressionSyntax(const TypeExpressionSyntax&) = delete;

		inline virtual ~TypeExpressionSyntax() = default;
	};
}
