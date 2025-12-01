#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/syntax/SyntaxToken.h>
#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxNode.h>

#include <shard/syntax/nodes/TypeSyntax.h>

#include <vector>

namespace shard::syntax::nodes
{
	class SHARD_API GenericTypeSyntax : public TypeSyntax
	{
	public:
		SyntaxToken OpenListToken;
		SyntaxToken CloseListToken;
		TypeSyntax* UnderlayingType = nullptr;
		std::vector<TypeSyntax*> TypeArguments;

		inline GenericTypeSyntax(TypeSyntax* underlaying, const SyntaxNode* parent)
			: TypeSyntax(SyntaxKind::GenericType, parent), UnderlayingType(underlaying) { }

		inline GenericTypeSyntax(const GenericTypeSyntax& other) = delete;

		inline virtual ~GenericTypeSyntax()
		{
			if (UnderlayingType != nullptr)
				delete UnderlayingType;

			for (TypeSyntax* arg : TypeArguments)
				delete arg;
		}

		std::wstring ToString() override;
	};
}
