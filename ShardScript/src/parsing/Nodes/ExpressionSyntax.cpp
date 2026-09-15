#include <shard/parsing/nodes/ExpressionSyntax.hpp>

#include <gmt/Arena.hpp>

using namespace shard;

ExpressionSyntax::ExpressionSyntax(const SyntaxKind kind, gmt::Ref<SyntaxNode> parent)
	: SyntaxNode(kind, parent) { }
