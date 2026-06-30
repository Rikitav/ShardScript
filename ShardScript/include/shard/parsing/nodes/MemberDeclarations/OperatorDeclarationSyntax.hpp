#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxToken.hpp>
#include <shard/parsing/SyntaxNode.hpp>

#include <shard/parsing/nodes/MemberDeclarationSyntax.hpp>
#include <shard/parsing/nodes/ParametersListSyntax.hpp>
#include <shard/parsing/nodes/StatementsBlockSyntax.hpp>
#include <shard/parsing/nodes/TypeSyntax.hpp>

#include <shard/parsing/MemberDeclarationInfo.hpp>

#include <vector>
#include <memory>

namespace shard
{
	class SHARD_API OperatorDeclarationSyntax : public MemberDeclarationSyntax
	{
	public:
		SyntaxToken OperatorToken;
		SyntaxToken Semicolon;
		std::unique_ptr<TypeSyntax> ReturnType = nullptr;
		std::unique_ptr<ParametersListSyntax> ParametersList = nullptr;
		std::unique_ptr<StatementsBlockSyntax> Body = nullptr;

		inline OperatorDeclarationSyntax(shard::MemberDeclarationInfo& info, const SyntaxToken& operatorToken, SyntaxNode* parent)
			: MemberDeclarationSyntax(SyntaxKind::OperatorDeclaration, parent), OperatorToken(operatorToken)
		{
			Attributes = std::move(info.Attributes);
			Modifiers = info.Modifiers;
			IdentifierToken = operatorToken;
			ReturnType = std::move(info.ReturnType);
			TypeParameters = std::move(info.Generics);
		}

		inline OperatorDeclarationSyntax(const OperatorDeclarationSyntax&) = delete;

		inline ~OperatorDeclarationSyntax() override = default;
	};
}
