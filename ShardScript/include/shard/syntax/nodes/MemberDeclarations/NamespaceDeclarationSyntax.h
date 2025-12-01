#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxNode.h>
#include <shard/syntax/SyntaxToken.h>

#include <shard/syntax/nodes/TypeDeclarationSyntax.h>
#include <shard/parsing/lexical/MemberDeclarationInfo.h>

#include <vector>

namespace shard::syntax::nodes
{
	class SHARD_API NamespaceDeclarationSyntax : public TypeDeclarationSyntax
	{
	public:
		std::vector<SyntaxToken> IdentifierTokens;

		inline NamespaceDeclarationSyntax(const SyntaxNode* parent)
			: TypeDeclarationSyntax(SyntaxKind::NamespaceDeclaration, parent) { }

		inline NamespaceDeclarationSyntax(shard::parsing::lexical::MemberDeclarationInfo& info, const SyntaxNode* parent) : TypeDeclarationSyntax(SyntaxKind::NamespaceDeclaration, parent)
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
