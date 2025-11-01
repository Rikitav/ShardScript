#pragma once
#include <shard/syntax/nodes/TypeSyntax.h>
#include <shard/syntax/SyntaxToken.h>
#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxNode.h>
#include <vector>

namespace shard::syntax::nodes
{
	class IdentifierNameTypeSyntax : public TypeSyntax
	{
	public:
		std::vector<SyntaxToken> Identifiers;

		inline IdentifierNameTypeSyntax(const SyntaxNode* parent)
			: TypeSyntax(SyntaxKind::IdentifierNameType, parent) { }
	};
}
