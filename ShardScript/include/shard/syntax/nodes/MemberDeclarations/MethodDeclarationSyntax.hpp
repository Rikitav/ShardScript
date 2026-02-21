#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/syntax/SyntaxKind.hpp>
#include <shard/syntax/SyntaxToken.hpp>
#include <shard/syntax/SyntaxNode.hpp>

#include <shard/syntax/nodes/MemberDeclarationSyntax.hpp>
#include <shard/syntax/nodes/ParametersListSyntax.hpp>
#include <shard/syntax/nodes/StatementsBlockSyntax.hpp>
#include <shard/syntax/nodes/TypeSyntax.hpp>

#include <shard/parsing/MemberDeclarationInfo.hpp>

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

		inline MethodDeclarationSyntax(shard::MemberDeclarationInfo& info, SyntaxNode *const parent) : MemberDeclarationSyntax(SyntaxKind::MethodDeclaration, parent)
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
