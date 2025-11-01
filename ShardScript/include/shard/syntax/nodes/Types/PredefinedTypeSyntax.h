#pragma once
#include <shard/syntax/nodes/TypeSyntax.h>
#include <shard/syntax/SyntaxToken.h>
#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxNode.h>

namespace shard::syntax::nodes
{
	class PredefinedTypeSyntax : public TypeSyntax
	{
	public:
		const SyntaxToken TypeToken;

		inline PredefinedTypeSyntax(const SyntaxToken typeToken, const SyntaxNode* parent)
			: TypeSyntax(SyntaxKind::PredefinedType, parent), TypeToken(typeToken) { }
	};
}
