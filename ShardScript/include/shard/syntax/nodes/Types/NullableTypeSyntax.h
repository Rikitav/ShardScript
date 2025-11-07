#pragma once
#include <shard/syntax/nodes/TypeSyntax.h>
#include <shard/syntax/SyntaxToken.h>
#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxNode.h>

namespace shard::syntax::nodes
{
	class NullableTypeSyntax : public TypeSyntax
	{
	public:
		TypeSyntax* UnderlayingType = nullptr;
		SyntaxToken QuestionToken;

		inline NullableTypeSyntax(TypeSyntax* underlaying, const SyntaxNode* parent)
			: TypeSyntax(SyntaxKind::NullableType, parent), UnderlayingType(underlaying) { }

		std::wstring ToString() override;
	};
}
