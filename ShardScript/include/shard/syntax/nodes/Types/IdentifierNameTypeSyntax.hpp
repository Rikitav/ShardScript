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

		inline IdentifierNameTypeSyntax(SyntaxNode *const parent)
			: TypeSyntax(SyntaxKind::IdentifierNameType, parent) { }

		inline IdentifierNameTypeSyntax(const IdentifierNameTypeSyntax& other) = delete;

		inline virtual ~IdentifierNameTypeSyntax()
		{

		}

		std::wstring ToString() override;
	};
}
