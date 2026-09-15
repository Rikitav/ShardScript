#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxNode.hpp>

#include <gmt/Ref.hpp>

namespace shard
{
	class SHARD_API BodySyntax : public SyntaxNode
	{
	public:
		BodySyntax(const SyntaxKind kind, gmt::Ref<SyntaxNode> parent);
		virtual ~BodySyntax();
	};
}