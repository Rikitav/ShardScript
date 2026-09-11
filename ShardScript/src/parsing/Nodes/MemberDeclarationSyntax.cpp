#include <shard/parsing/nodes/MemberDeclarationSyntax.hpp>

using namespace shard;

MemberDeclarationSyntax::MemberDeclarationSyntax(const SyntaxKind kind, SyntaxNode* parent)
	: SyntaxNode(kind, parent) { }
