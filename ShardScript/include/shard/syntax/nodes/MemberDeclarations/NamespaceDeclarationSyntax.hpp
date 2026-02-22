#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/syntax/SyntaxKind.hpp>
#include <shard/syntax/SyntaxNode.hpp>
#include <shard/syntax/SyntaxToken.hpp>

#include <shard/syntax/nodes/TypeDeclarationSyntax.hpp>
#include <shard/parsing/MemberDeclarationInfo.hpp>

#include <vector>

namespace shard
{
	class SHARD_API NamespaceDeclarationSyntax : public TypeDeclarationSyntax
	{
	public:
		std::vector<SyntaxToken> IdentifierTokens;

		inline NamespaceDeclarationSyntax(SyntaxNode *const parent)
			: TypeDeclarationSyntax(SyntaxKind::NamespaceDeclaration, parent) { }

		inline NamespaceDeclarationSyntax(shard::MemberDeclarationInfo& info, SyntaxNode *const parent) : TypeDeclarationSyntax(SyntaxKind::NamespaceDeclaration, parent)
		{
			Modifiers = info.Modifiers;
			IdentifierToken = info.Identifier;
		}

		inline NamespaceDeclarationSyntax(const NamespaceDeclarationSyntax& other) = delete;

		inline virtual ~NamespaceDeclarationSyntax()
		{

		}
	};
}
