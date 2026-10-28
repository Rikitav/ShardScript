#include <shard/parsing/nodes/BlockSyntax.hpp>

#include <gmt/Arena.hpp>

using namespace shard;

BlockSyntax::BlockSyntax(const SyntaxKind kind, SyntaxNode* parent)
	: SyntaxNode(kind, parent) { }