#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/parsing/lexical/MemberDeclarationInfo.h>
#include <shard/syntax/nodes/TypeDeclarationSyntax.h>
#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxNode.h>

namespace shard::syntax::nodes
{
	class SHARD_API ClassDeclarationSyntax : public TypeDeclarationSyntax
	{
	public:
		inline ClassDeclarationSyntax(const SyntaxNode* parent)
			: TypeDeclarationSyntax(SyntaxKind::ClassDeclaration, parent) { }

		inline ClassDeclarationSyntax(shard::parsing::lexical::MemberDeclarationInfo& info, const SyntaxNode* parent) : TypeDeclarationSyntax(SyntaxKind::ClassDeclaration, parent)
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
