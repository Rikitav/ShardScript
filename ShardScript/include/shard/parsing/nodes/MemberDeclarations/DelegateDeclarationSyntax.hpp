#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/parsing/MemberDeclarationInfo.hpp>

#include <shard/parsing/nodes/MemberDeclarationSyntax.hpp>
#include <shard/parsing/nodes/ParametersListSyntax.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxNode.hpp>
#include <shard/parsing/SyntaxToken.hpp>

#include <memory>

namespace shard
{
	class SHARD_API DelegateDeclarationSyntax : public MemberDeclarationSyntax
	{
	public:
		SyntaxToken DelegateToken;
		SyntaxToken Semicolon;

		std::unique_ptr<TypeSyntax> ReturnType = nullptr;
		std::unique_ptr<ParametersListSyntax> ParametersList = nullptr;

		inline DelegateDeclarationSyntax(SyntaxNode* parent)
			: MemberDeclarationSyntax(SyntaxKind::DelegateDeclaration, parent) { }

		inline DelegateDeclarationSyntax(shard::MemberDeclarationInfo& info, SyntaxNode* parent) : MemberDeclarationSyntax(SyntaxKind::DelegateDeclaration, parent)
		{
			Attributes = std::move(info.Attributes);
			Modifiers = info.Modifiers;
			DelegateToken = info.DeclareType;
			ReturnType = std::move(info.ReturnType);
			IdentifierToken = info.Identifier;
			TypeParameters = std::move(info.Generics);
		}

		inline DelegateDeclarationSyntax(const DelegateDeclarationSyntax& other) = delete;

		inline virtual ~DelegateDeclarationSyntax() = default;
	};
}
