#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/syntax/SyntaxToken.h>
#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxNode.h>

#include <shard/syntax/nodes/TypeSyntax.h>
#include <shard/syntax/nodes/TypeArgumentsListSyntax.h>

#include <vector>

namespace shard
{
	class SHARD_API GenericTypeSyntax : public TypeSyntax
	{
	public:
		TypeSyntax* UnderlayingType = nullptr;
		TypeArgumentsListSyntax* Arguments = nullptr;

		inline GenericTypeSyntax(TypeSyntax* underlaying, SyntaxNode *const parent)
			: TypeSyntax(SyntaxKind::GenericType, parent), UnderlayingType(underlaying) { }

		inline GenericTypeSyntax(const GenericTypeSyntax& other) = delete;

		inline virtual ~GenericTypeSyntax()
		{
			if (UnderlayingType != nullptr)
				delete UnderlayingType;

			if (Arguments != nullptr)
				delete Arguments;
		}

		std::wstring ToString() override;
	};
}
