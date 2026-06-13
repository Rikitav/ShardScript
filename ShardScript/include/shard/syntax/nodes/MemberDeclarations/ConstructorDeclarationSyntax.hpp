#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/syntax/SyntaxKind.hpp>
#include <shard/syntax/SyntaxToken.hpp>
#include <shard/syntax/SyntaxNode.hpp>

#include <shard/syntax/nodes/MemberDeclarationSyntax.hpp>
#include <shard/syntax/nodes/ParametersListSyntax.hpp>
#include <shard/syntax/nodes/StatementsBlockSyntax.hpp>

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

		inline ConstructorDeclarationSyntax(shard::MemberDeclarationInfo& info, SyntaxNode *const parent) : MemberDeclarationSyntax(SyntaxKind::ConstructorDeclaration, parent)
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
