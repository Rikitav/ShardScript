#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/syntax/SyntaxToken.h>
#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxNode.h>

#include <shard/syntax/nodes/TypeSyntax.h>

namespace shard
{
	class SHARD_API NullableTypeSyntax : public TypeSyntax
	{
	public:
		TypeSyntax* UnderlayingType = nullptr;
		SyntaxToken QuestionToken;

		inline NullableTypeSyntax(TypeSyntax* underlaying, SyntaxNode *const parent)
			: TypeSyntax(SyntaxKind::NullableType, parent), UnderlayingType(underlaying) { }

		inline NullableTypeSyntax(const NullableTypeSyntax& other) = delete;

		inline virtual ~NullableTypeSyntax()
		{
			if (UnderlayingType != nullptr)
				delete UnderlayingType;
		}

		std::wstring ToString() override;
	};
}
