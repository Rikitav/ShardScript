#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/parsing/MemberDeclarationInfo.hpp>
#include <shard/parsing/nodes/TypeDeclarationSyntax.hpp>
#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxNode.hpp>

namespace shard
{
	class SHARD_API ClassDeclarationSyntax : public TypeDeclarationSyntax
	{
	public:
		inline ClassDeclarationSyntax(SyntaxNode* parent)
			: TypeDeclarationSyntax(SyntaxKind::ClassDeclaration, parent) { }

		inline ClassDeclarationSyntax(shard::MemberDeclarationInfo& info, SyntaxNode* parent) : TypeDeclarationSyntax(SyntaxKind::ClassDeclaration, parent)
		{
			Attributes = std::move(info.Attributes);
			Modifiers = info.Modifiers;
			IdentifierToken = info.Identifier;
			TypeParameters = std::move(info.Generics);
		}

		inline ClassDeclarationSyntax(const ClassDeclarationSyntax& other) = delete;

		inline virtual ~ClassDeclarationSyntax() = default;
	};
}
