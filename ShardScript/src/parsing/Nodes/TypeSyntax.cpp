#include <shard/parsing/nodes/TypeSyntax.hpp>

#include <gmt/Arena.hpp>

using namespace shard;

TypeSyntax::TypeSyntax(const SyntaxKind kind, gmt::Ref<SyntaxNode> parent)
	: ExpressionSyntax(kind, parent) { }
