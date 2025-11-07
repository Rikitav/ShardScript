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
		TypeSyntax* UnderlayingType = nullptr;
		SyntaxToken OpenListToken;
		SyntaxToken CloseListToken;
		std::vector<TypeSyntax*> TypeArguments;

		inline GenericTypeSyntax(const SyntaxNode* parent, TypeSyntax* underlaying)
			: TypeSyntax(SyntaxKind::GenericType, parent), UnderlayingType(underlaying) { }

		std::wstring ToString() override;
	};
}
