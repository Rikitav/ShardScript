#include <shard/parsing/nodes/StatementSyntax.hpp>

#include <gmt/Arena.hpp>

using namespace shard;

StatementSyntax::StatementSyntax(const SyntaxKind kind, gmt::Ref<SyntaxNode> parent)
	: SyntaxNode(kind, parent) { }
