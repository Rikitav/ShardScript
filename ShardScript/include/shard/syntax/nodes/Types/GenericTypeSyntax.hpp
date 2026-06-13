#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/syntax/SyntaxToken.hpp>
#include <shard/syntax/SyntaxKind.hpp>
#include <shard/syntax/SyntaxNode.hpp>

#include <shard/syntax/nodes/TypeSyntax.hpp>
#include <shard/syntax/nodes/TypeArgumentsListSyntax.hpp>

#include <vector>
#include <memory>

namespace shard
{
	class SHARD_API GenericTypeSyntax : public TypeSyntax
	{
	public:
		std::unique_ptr<TypeSyntax> UnderlayingType = nullptr;
		std::unique_ptr<TypeArgumentsListSyntax> Arguments = nullptr;

		inline GenericTypeSyntax(std::unique_ptr<TypeSyntax> underlaying, SyntaxNode *const parent)
			: TypeSyntax(SyntaxKind::GenericType, parent), UnderlayingType(std::move(underlaying)) { }

		inline GenericTypeSyntax(const GenericTypeSyntax& other) = delete;

		inline virtual ~GenericTypeSyntax() = default;

		std::wstring ToString() override;
	};
}
