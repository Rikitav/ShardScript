#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/syntax/SyntaxToken.h>
#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxNode.h>

#include <shard/syntax/nodes/TypeSyntax.h>

#include <vector>

namespace shard
{
	class SHARD_API IdentifierNameTypeSyntax : public TypeSyntax
	{
	public:
		SyntaxToken Identifier;

		inline IdentifierNameTypeSyntax(const SyntaxNode* parent)
			: TypeSyntax(SyntaxKind::IdentifierNameType, parent) { }

		inline IdentifierNameTypeSyntax(const IdentifierNameTypeSyntax& other) = delete;

		inline virtual ~IdentifierNameTypeSyntax()
		{

		}

		std::wstring ToString() override;
	};
}
