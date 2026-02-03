#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/syntax/nodes/TypeDeclarationSyntax.h>
#include <shard/parsing/MemberDeclarationInfo.h>

#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxNode.h>

namespace shard
{
	class SHARD_API StructDeclarationSyntax : public TypeDeclarationSyntax
	{
	public:
		inline StructDeclarationSyntax(SyntaxNode *const parent)
			: TypeDeclarationSyntax(SyntaxKind::StructDeclaration, parent) { }

		inline StructDeclarationSyntax(shard::MemberDeclarationInfo& info, SyntaxNode *const parent) : TypeDeclarationSyntax(SyntaxKind::StructDeclaration, parent)
		{
			Modifiers = info.Modifiers;
			IdentifierToken = info.Identifier;
			TypeParameters = info.Generics;
		}

		inline StructDeclarationSyntax(const StructDeclarationSyntax& other) = delete;

		inline virtual ~StructDeclarationSyntax()
		{

		}
	};
}
