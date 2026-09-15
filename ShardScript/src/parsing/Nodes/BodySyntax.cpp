#include <shard/parsing/nodes/BodySyntax.hpp>

#include <gmt/Arena.hpp>

using namespace shard;

BodySyntax::BodySyntax(const SyntaxKind kind, gmt::Ref<SyntaxNode> parent)
	: SyntaxNode(kind, parent) { }
