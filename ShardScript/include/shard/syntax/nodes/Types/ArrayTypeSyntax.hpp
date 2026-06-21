#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/syntax/SyntaxToken.hpp>
#include <shard/syntax/SyntaxKind.hpp>
#include <shard/syntax/SyntaxNode.hpp>

#include <shard/syntax/nodes/TypeSyntax.hpp>

#include <string>
#include <memory>

namespace shard
{
	class SHARD_API ArrayTypeSyntax : public TypeSyntax
	{
	public:
		std::unique_ptr<TypeSyntax> UnderlayingType = nullptr;
		SyntaxToken OpenSquareToken;
		SyntaxToken CloseSquareToken;
		int Rank = 1;

		inline ArrayTypeSyntax(std::unique_ptr<TypeSyntax> underlaying, SyntaxNode* parent)
			: TypeSyntax(SyntaxKind::ArrayType, parent), UnderlayingType(std::move(underlaying)) { }

		inline ArrayTypeSyntax(const ArrayTypeSyntax& other) = delete;

		inline virtual ~ArrayTypeSyntax() = default;

		std::wstring ToString() override;
	};
}
