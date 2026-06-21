#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/syntax/SyntaxToken.hpp>
#include <shard/syntax/SyntaxKind.hpp>
#include <shard/syntax/SyntaxNode.hpp>

#include <shard/syntax/nodes/TypeSyntax.hpp>

#include <vector>

namespace shard
{
	class SHARD_API IdentifierNameTypeSyntax : public TypeSyntax
	{
	public:
		SyntaxToken Identifier;

		inline IdentifierNameTypeSyntax(SyntaxNode* parent)
			: TypeSyntax(SyntaxKind::IdentifierNameType, parent) { }

		inline IdentifierNameTypeSyntax(const IdentifierNameTypeSyntax& other) = delete;

		inline virtual ~IdentifierNameTypeSyntax() = default;

		std::wstring ToString() override;
	};
}
