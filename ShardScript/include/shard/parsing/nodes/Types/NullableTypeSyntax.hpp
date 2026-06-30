#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/parsing/SyntaxToken.hpp>
#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxNode.hpp>

#include <shard/parsing/nodes/TypeSyntax.hpp>

#include <memory>

namespace shard
{
	class SHARD_API NullableTypeSyntax : public TypeSyntax
	{
	public:
		std::unique_ptr<TypeSyntax> UnderlayingType = nullptr;
		SyntaxToken QuestionToken;

		inline NullableTypeSyntax(std::unique_ptr<TypeSyntax> underlaying, SyntaxNode* parent)
			: TypeSyntax(SyntaxKind::NullableType, parent), UnderlayingType(std::move(underlaying)) { }

		inline NullableTypeSyntax(const NullableTypeSyntax& other) = delete;

		inline virtual ~NullableTypeSyntax() = default;

		std::wstring ToString() override;
	};
}
