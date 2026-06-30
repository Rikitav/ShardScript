#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/parsing/nodes/TypeDeclarationSyntax.hpp>
#include <shard/parsing/MemberDeclarationInfo.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxNode.hpp>

namespace shard
{
	class SHARD_API StructDeclarationSyntax : public TypeDeclarationSyntax
	{
	public:
		inline StructDeclarationSyntax(SyntaxNode* parent)
			: TypeDeclarationSyntax(SyntaxKind::StructDeclaration, parent) { }

		inline StructDeclarationSyntax(shard::MemberDeclarationInfo& info, SyntaxNode* parent) : TypeDeclarationSyntax(SyntaxKind::StructDeclaration, parent)
		{
			Attributes = std::move(info.Attributes);
			Modifiers = info.Modifiers;
			IdentifierToken = info.Identifier;
			TypeParameters = std::move(info.Generics);
		}

		inline StructDeclarationSyntax(const StructDeclarationSyntax& other) = delete;

		inline virtual ~StructDeclarationSyntax() = default;
	};
}
