#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxNode.hpp>
#include <shard/parsing/SyntaxKind.hpp>

#include <gmt/Ref.hpp>

namespace shard
{
	class SHARD_API StatementSyntax : public SyntaxNode
	{
	public:
		StatementSyntax(const SyntaxKind kind, gmt::Ref<SyntaxNode> parent);
		virtual ~StatementSyntax() = default;
	};
}
