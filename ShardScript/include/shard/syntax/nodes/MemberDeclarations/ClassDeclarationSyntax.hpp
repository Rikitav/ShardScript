#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/parsing/MemberDeclarationInfo.hpp>
#include <shard/syntax/nodes/TypeDeclarationSyntax.hpp>
#include <shard/syntax/SyntaxKind.hpp>
#include <shard/syntax/SyntaxNode.hpp>

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
