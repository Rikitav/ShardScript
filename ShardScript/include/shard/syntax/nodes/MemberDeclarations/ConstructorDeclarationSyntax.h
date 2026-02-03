#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxToken.h>
#include <shard/syntax/SyntaxNode.h>

#include <shard/syntax/nodes/MemberDeclarationSyntax.h>
#include <shard/syntax/nodes/ParametersListSyntax.h>
#include <shard/syntax/nodes/StatementsBlockSyntax.h>

#include <shard/parsing/MemberDeclarationInfo.h>
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
