#pragma once
#include <shard/syntax/SyntaxToken.h>
#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxNode.h>
#include <shard/syntax/nodes/TypeSyntax.h>
#include <string>

namespace shard::syntax::nodes
{
	class ArrayTypeSyntax : public TypeSyntax
	{
	public:
		TypeSyntax* UnderlayingType = nullptr;
		SyntaxToken OpenSquareToken;
		SyntaxToken CloseSquareToken;
		int Rank = 1;

		inline ArrayTypeSyntax(TypeSyntax* underlaying, const SyntaxNode* parent)
			: TypeSyntax(SyntaxKind::ArrayType, parent), UnderlayingType(underlaying) { }
		
		std::wstring ToString() override;
	};
}
