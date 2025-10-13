#pragma once
#include <shard/syntax/nodes/TypeDeclarationSyntax.h>
#include <shard/parsing/structures/MemberDeclarationInfo.h>

using namespace shard::syntax::structures;

namespace shard::syntax::nodes
{
	class NamespaceDeclarationSyntax : public TypeDeclarationSyntax
	{
	public:
		NamespaceDeclarationSyntax()
			: TypeDeclarationSyntax(SyntaxKind::NamespaceDeclaration) {
		}
	};

	class ClassDeclarationSyntax : public TypeDeclarationSyntax
	{
	public:
		ClassDeclarationSyntax()
			: TypeDeclarationSyntax(SyntaxKind::ClassDeclaration) {
		}

		ClassDeclarationSyntax(MemberDeclarationInfo& info) : TypeDeclarationSyntax(SyntaxKind::ClassDeclaration)
		{
			Modifiers = info.Modifiers;
			Identifier = info.Identifier;
		}
	};

	class StructDeclarationSyntax : public TypeDeclarationSyntax
	{
	public:
		StructDeclarationSyntax()
			: TypeDeclarationSyntax(SyntaxKind::StructDeclaration) {
		}
	};

	class InterfaceDeclarationSyntax : public TypeDeclarationSyntax
	{
	public:
		InterfaceDeclarationSyntax()
			: TypeDeclarationSyntax(SyntaxKind::InterfaceDeclaration) {
		}
	};
}