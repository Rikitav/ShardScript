#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/syntax/SyntaxToken.h>
#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxNode.h>

#include <shard/syntax/nodes/TypeSyntax.h>

#include <string>

namespace shard
{
	class SHARD_API ArrayTypeSyntax : public TypeSyntax
	{
	public:
		TypeSyntax* UnderlayingType = nullptr;
		SyntaxToken OpenSquareToken;
		SyntaxToken CloseSquareToken;
		int Rank = 1;

		inline ArrayTypeSyntax(TypeSyntax* underlaying, const SyntaxNode* parent)
			: TypeSyntax(SyntaxKind::ArrayType, parent), UnderlayingType(underlaying) { }

		inline ArrayTypeSyntax(const ArrayTypeSyntax& other) = delete;

		inline virtual ~ArrayTypeSyntax()
		{
			if (UnderlayingType != nullptr)
				delete UnderlayingType;
		}

		std::wstring ToString() override;
	};
}
