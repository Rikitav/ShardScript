#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxToken.h>
#include <shard/syntax/SyntaxNode.h>

#include <shard/syntax/nodes/MemberDeclarationSyntax.h>
#include <shard/syntax/nodes/ParametersListSyntax.h>
#include <shard/syntax/nodes/StatementsBlockSyntax.h>
#include <shard/syntax/nodes/TypeSyntax.h>

#include <shard/parsing/MemberDeclarationInfo.h>

#include <vector>

namespace shard
{
	class SHARD_API MethodDeclarationSyntax : public MemberDeclarationSyntax
	{
	public:
		SyntaxToken Semicolon;
		TypeSyntax* ReturnType = nullptr;
		ParametersListSyntax* Params = nullptr;
		StatementsBlockSyntax* Body = nullptr;

		inline MethodDeclarationSyntax(shard::MemberDeclarationInfo& info, const SyntaxNode* parent) : MemberDeclarationSyntax(SyntaxKind::MethodDeclaration, parent)
		{
			Modifiers = info.Modifiers;
			IdentifierToken = info.Identifier;
			ReturnType = info.ReturnType;
			TypeParameters = info.Generics;
		}

		inline MethodDeclarationSyntax(const MethodDeclarationSyntax&) = delete;

		inline ~MethodDeclarationSyntax() override
		{
			if (ReturnType != nullptr)
				delete ReturnType;

			if (Params != nullptr)
				delete Params;

			if (Body != nullptr)
				delete Body;
		}
	};
}
