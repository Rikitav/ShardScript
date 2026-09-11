#include <shard/parsing/nodes/AttributeSyntax.hpp>

using namespace shard;

AttributeSyntax::AttributeSyntax(SyntaxNode* parent)
	: SyntaxNode(SyntaxKind::Attribute, parent) { }
