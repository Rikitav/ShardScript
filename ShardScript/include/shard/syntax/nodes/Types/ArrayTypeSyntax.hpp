#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/syntax/SyntaxToken.hpp>
#include <shard/syntax/SyntaxKind.hpp>
#include <shard/syntax/SyntaxNode.hpp>

#include <shard/syntax/nodes/TypeSyntax.hpp>

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

		inline ArrayTypeSyntax(TypeSyntax* underlaying, SyntaxNode *const parent)
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
