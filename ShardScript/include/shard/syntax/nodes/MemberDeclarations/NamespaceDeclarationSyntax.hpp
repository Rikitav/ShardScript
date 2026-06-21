#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/syntax/SyntaxKind.hpp>
#include <shard/syntax/SyntaxNode.hpp>
#include <shard/syntax/SyntaxToken.hpp>

#include <shard/syntax/nodes/MemberDeclarationSyntax.hpp>
#include <shard/parsing/MemberDeclarationInfo.hpp>

#include <vector>

namespace shard
{
	class SHARD_API NamespaceDeclarationSyntax : public MemberDeclarationSyntax
	{
	public:
		std::vector<SyntaxToken> IdentifierTokens;
		SyntaxToken SemicolonToken;

		inline NamespaceDeclarationSyntax(SyntaxNode* parent)
			: MemberDeclarationSyntax(SyntaxKind::NamespaceDeclaration, parent) { }

		inline NamespaceDeclarationSyntax(shard::MemberDeclarationInfo& info, SyntaxNode* parent)
			: MemberDeclarationSyntax(SyntaxKind::NamespaceDeclaration, parent)
		{
			Attributes = std::move(info.Attributes);
			Modifiers = info.Modifiers;
			IdentifierToken = info.Identifier;
		}

		inline NamespaceDeclarationSyntax(const NamespaceDeclarationSyntax& other) = delete;

		inline virtual ~NamespaceDeclarationSyntax() = default;
	};
}
