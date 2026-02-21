#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/syntax/SyntaxToken.hpp>
#include <shard/syntax/SyntaxKind.hpp>
#include <shard/syntax/SyntaxNode.hpp>

#include <shard/syntax/nodes/TypeSyntax.hpp>
#include <shard/syntax/nodes/TypeArgumentsListSyntax.hpp>

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
