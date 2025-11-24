#pragma once
#include <shard/syntax/nodes/TypeSyntax.h>
#include <shard/syntax/SyntaxToken.h>
#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxNode.h>
#include <vector>

namespace shard::syntax::nodes
{
	class GenericTypeSyntax : public TypeSyntax
	{
	public:
		SyntaxToken OpenListToken;
		SyntaxToken CloseListToken;
		TypeSyntax* UnderlayingType = nullptr;
		std::vector<TypeSyntax*> TypeArguments;

		inline GenericTypeSyntax(TypeSyntax* underlaying, const SyntaxNode* parent)
			: TypeSyntax(SyntaxKind::GenericType, parent), UnderlayingType(underlaying) { }

		std::wstring ToString() override;
	};
}
