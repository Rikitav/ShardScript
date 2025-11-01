#pragma once
#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxNode.h>
#include <shard/syntax/nodes/TypeDeclarationSyntax.h>
#include <shard/parsing/lexical/MemberDeclarationInfo.h>

namespace shard::syntax::nodes
{
	class NamespaceDeclarationSyntax : public TypeDeclarationSyntax
	{
	public:
		inline NamespaceDeclarationSyntax(const SyntaxNode* parent)
			: TypeDeclarationSyntax(SyntaxKind::NamespaceDeclaration, parent) { }

		inline NamespaceDeclarationSyntax(const NamespaceDeclarationSyntax& other)
			: TypeDeclarationSyntax(other) { }

		inline NamespaceDeclarationSyntax(shard::parsing::lexical::MemberDeclarationInfo& info, const SyntaxNode* parent) : TypeDeclarationSyntax(SyntaxKind::NamespaceDeclaration, parent)
		{
			Modifiers = info.Modifiers;
			IdentifierToken = info.Identifier;
		}

		inline virtual ~NamespaceDeclarationSyntax()
		{

		}
	};
}
