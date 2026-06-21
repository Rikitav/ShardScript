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
#include <memory>

namespace shard
{
	class SHARD_API MethodDeclarationSyntax : public MemberDeclarationSyntax
	{
	public:
		SyntaxToken Semicolon;
		std::unique_ptr<TypeSyntax> ReturnType = nullptr;
		std::unique_ptr<ParametersListSyntax> ParametersList = nullptr;
		std::unique_ptr<StatementsBlockSyntax> Body = nullptr;

		inline MethodDeclarationSyntax(shard::MemberDeclarationInfo& info, SyntaxNode* parent) : MemberDeclarationSyntax(SyntaxKind::MethodDeclaration, parent)
		{
			Attributes = std::move(info.Attributes);
			Modifiers = info.Modifiers;
			IdentifierToken = info.Identifier;
			ReturnType = std::move(info.ReturnType);
			TypeParameters = std::move(info.Generics);
		}

		inline MethodDeclarationSyntax(const MethodDeclarationSyntax&) = delete;

		inline ~MethodDeclarationSyntax() override = default;
	};
}
