#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/parsing/MemberDeclarationInfo.hpp>
#include <shard/syntax/nodes/TypeDeclarationSyntax.hpp>
#include <shard/syntax/SyntaxKind.hpp>
#include <shard/syntax/SyntaxNode.hpp>

namespace shard
{
	class SHARD_API InterfaceDeclarationSyntax : public TypeDeclarationSyntax
	{
	public:
		inline InterfaceDeclarationSyntax(SyntaxNode *const parent)
			: TypeDeclarationSyntax(SyntaxKind::InterfaceDeclaration, parent) { }

		inline InterfaceDeclarationSyntax(shard::MemberDeclarationInfo& info, SyntaxNode *const parent) : TypeDeclarationSyntax(SyntaxKind::InterfaceDeclaration, parent)
		{
			Modifiers = info.Modifiers;
			IdentifierToken = info.Identifier;
			TypeParameters = info.Generics;
		}

		inline InterfaceDeclarationSyntax(const InterfaceDeclarationSyntax& other) = delete;

		inline virtual ~InterfaceDeclarationSyntax()
		{

		}
	};
}
