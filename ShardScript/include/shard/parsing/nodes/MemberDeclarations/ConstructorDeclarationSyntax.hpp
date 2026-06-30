#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxToken.hpp>
#include <shard/parsing/SyntaxNode.hpp>

#include <shard/parsing/nodes/MemberDeclarationSyntax.hpp>
#include <shard/parsing/nodes/ParametersListSyntax.hpp>
#include <shard/parsing/nodes/StatementsBlockSyntax.hpp>

#include <shard/parsing/MemberDeclarationInfo.hpp>

#include <memory>
#include <vector>

namespace shard
{
	class SHARD_API ConstructorDeclarationSyntax : public MemberDeclarationSyntax
	{
	public:
		SyntaxToken Semicolon;
		std::unique_ptr<StatementsBlockSyntax> Body = nullptr;
		std::unique_ptr<ParametersListSyntax> ParametersList = nullptr;

		inline ConstructorDeclarationSyntax(shard::MemberDeclarationInfo& info, SyntaxNode* parent) : MemberDeclarationSyntax(SyntaxKind::ConstructorDeclaration, parent)
		{
			Attributes = std::move(info.Attributes);
			Modifiers = info.Modifiers;
			IdentifierToken = info.Identifier;
			TypeParameters = std::move(info.Generics);
		}

		inline ConstructorDeclarationSyntax(const ConstructorDeclarationSyntax&) = delete;

		inline virtual ~ConstructorDeclarationSyntax() = default;
	};
}
