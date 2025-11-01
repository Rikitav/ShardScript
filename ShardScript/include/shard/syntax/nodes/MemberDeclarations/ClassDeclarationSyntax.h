#pragma once
#include <shard/syntax/nodes/TypeDeclarationSyntax.h>
#include <shard/parsing/lexical/MemberDeclarationInfo.h>
#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxNode.h>

namespace shard::syntax::nodes
{
	class ClassDeclarationSyntax : public TypeDeclarationSyntax
	{
	public:
		inline ClassDeclarationSyntax(const SyntaxNode* parent)
			: TypeDeclarationSyntax(SyntaxKind::ClassDeclaration, parent) { }

		inline ClassDeclarationSyntax(const ClassDeclarationSyntax& other)
			: TypeDeclarationSyntax(other) { }

		inline ClassDeclarationSyntax(shard::parsing::lexical::MemberDeclarationInfo& info, const SyntaxNode* parent) : TypeDeclarationSyntax(SyntaxKind::ClassDeclaration, parent)
		{
			Modifiers = info.Modifiers;
			IdentifierToken = info.Identifier;
		}

		inline virtual ~ClassDeclarationSyntax()
		{

		}
	};
}
