#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/parsing/SyntaxToken.hpp>
#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxNode.hpp>

#include <shard/parsing/nodes/TypeSyntax.hpp>

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
