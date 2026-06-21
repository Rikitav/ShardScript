#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/parsing/MemberDeclarationInfo.hpp>

#include <shard/syntax/nodes/MemberDeclarationSyntax.hpp>
#include <shard/syntax/nodes/ParametersListSyntax.hpp>

#include <shard/syntax/SyntaxKind.hpp>
#include <shard/syntax/SyntaxNode.hpp>
#include <shard/syntax/SyntaxToken.hpp>

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
