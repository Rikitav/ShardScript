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
		SyntaxToken Identifier;

		inline IdentifierNameTypeSyntax(const SyntaxNode* parent)
			: TypeSyntax(SyntaxKind::IdentifierNameType, parent) { }

		std::wstring ToString() override;
	};
}
