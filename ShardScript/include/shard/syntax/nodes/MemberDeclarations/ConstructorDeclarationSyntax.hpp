#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/syntax/SyntaxKind.hpp>
#include <shard/syntax/SyntaxToken.hpp>
#include <shard/syntax/SyntaxNode.hpp>

#include <shard/syntax/nodes/MemberDeclarationSyntax.hpp>
#include <shard/syntax/nodes/ParametersListSyntax.hpp>
#include <shard/syntax/nodes/StatementsBlockSyntax.hpp>

#include <shard/parsing/MemberDeclarationInfo.hpp>
#include <vector>

namespace shard
{
	class SHARD_API ConstructorDeclarationSyntax : public MemberDeclarationSyntax
	{
	public:
		SyntaxToken Semicolon;
		StatementsBlockSyntax* Body = nullptr;
		ParametersListSyntax* Params = nullptr;

		inline ConstructorDeclarationSyntax(shard::MemberDeclarationInfo& info, SyntaxNode *const parent) : MemberDeclarationSyntax(SyntaxKind::ConstructorDeclaration, parent)
		{
			Modifiers = info.Modifiers;
			IdentifierToken = info.Identifier;
			TypeParameters = info.Generics;
		}

		inline ConstructorDeclarationSyntax(const ConstructorDeclarationSyntax&) = delete;

		inline ~ConstructorDeclarationSyntax() override
		{
			if (Body != nullptr)
				delete Body;

			if (Params != nullptr)
				delete Params;
		}
	};
}
