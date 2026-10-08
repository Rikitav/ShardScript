#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxNode.hpp>

#include <gmt/Ref.hpp>

namespace shard
{
	class SHARD_API BlockSyntax : public SyntaxNode
	{
	public:
		BlockSyntax(const SyntaxKind kind, gmt::Ref<SyntaxNode> parent);
		virtual ~BlockSyntax() = default;
	};
}