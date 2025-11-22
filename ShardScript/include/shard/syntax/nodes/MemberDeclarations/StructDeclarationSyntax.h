#pragma once
#include <shard/syntax/nodes/TypeDeclarationSyntax.h>
#include <shard/parsing/lexical/MemberDeclarationInfo.h>
#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxNode.h>

namespace shard::syntax::nodes
{
	class StructDeclarationSyntax : public TypeDeclarationSyntax
	{
	public:
		inline StructDeclarationSyntax(const SyntaxNode* parent)
			: TypeDeclarationSyntax(SyntaxKind::StructDeclaration, parent) { }

		inline StructDeclarationSyntax(const StructDeclarationSyntax& other)
			: TypeDeclarationSyntax(other) { }

		inline StructDeclarationSyntax(shard::parsing::lexical::MemberDeclarationInfo& info, const SyntaxNode* parent) : TypeDeclarationSyntax(SyntaxKind::StructDeclaration, parent)
		{
			Modifiers = info.Modifiers;
			IdentifierToken = info.Identifier;
		}

		inline virtual ~StructDeclarationSyntax()
		{

		}
	};
}
