#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxNode.h>
#include <shard/syntax/SyntaxToken.h>

#include <shard/syntax/nodes/TypeDeclarationSyntax.h>
#include <shard/parsing/MemberDeclarationInfo.h>

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
