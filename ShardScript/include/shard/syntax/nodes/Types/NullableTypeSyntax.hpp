#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/syntax/SyntaxToken.hpp>
#include <shard/syntax/SyntaxKind.hpp>
#include <shard/syntax/SyntaxNode.hpp>

#include <shard/syntax/nodes/TypeSyntax.hpp>

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
