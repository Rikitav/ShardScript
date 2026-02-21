#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/syntax/nodes/TypeDeclarationSyntax.hpp>
#include <shard/parsing/MemberDeclarationInfo.hpp>

#include <shard/syntax/SyntaxKind.hpp>
#include <shard/syntax/SyntaxNode.hpp>

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
