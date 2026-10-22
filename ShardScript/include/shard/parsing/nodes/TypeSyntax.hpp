#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/nodes/ExpressionSyntax.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxNode.hpp>

#include <gmt/Containers.hpp>

#include <string>

namespace shard
{
	class SHARD_API TypeSyntax : public ExpressionSyntax
	{
	public:
		TypeSyntax(const SyntaxKind kind, gmt::Ref<SyntaxNode> parent);
		virtual ~TypeSyntax() = default;

		virtual gmt::wstring get_qualifier() const = 0;
	};
}
