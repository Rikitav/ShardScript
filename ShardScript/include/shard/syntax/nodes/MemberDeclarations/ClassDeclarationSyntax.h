#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/parsing/MemberDeclarationInfo.h>
#include <shard/syntax/nodes/TypeDeclarationSyntax.h>
#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxNode.h>

namespace shard
{
	class SHARD_API ClassDeclarationSyntax : public TypeDeclarationSyntax
	{
	public:
		inline ClassDeclarationSyntax(SyntaxNode *const parent)
			: TypeDeclarationSyntax(SyntaxKind::ClassDeclaration, parent) { }

		inline ClassDeclarationSyntax(shard::MemberDeclarationInfo& info, SyntaxNode *const parent) : TypeDeclarationSyntax(SyntaxKind::ClassDeclaration, parent)
		{
			Modifiers = info.Modifiers;
			IdentifierToken = info.Identifier;
			TypeParameters = info.Generics;
		}

		inline ClassDeclarationSyntax(const ClassDeclarationSyntax& other) = delete;

		inline virtual ~ClassDeclarationSyntax()
		{

		}
	};
}
