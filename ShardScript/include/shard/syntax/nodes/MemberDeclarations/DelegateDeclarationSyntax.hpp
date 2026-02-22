#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/parsing/MemberDeclarationInfo.hpp>

#include <shard/syntax/nodes/MemberDeclarationSyntax.hpp>
#include <shard/syntax/nodes/ParametersListSyntax.hpp>

#include <shard/syntax/SyntaxKind.hpp>
#include <shard/syntax/SyntaxNode.hpp>
#include <shard/syntax/SyntaxToken.hpp>

namespace shard
{
	class SHARD_API DelegateDeclarationSyntax : public MemberDeclarationSyntax
	{
	public:
		SyntaxToken DelegateToken;
		SyntaxToken Semicolon;
		TypeSyntax* ReturnType = nullptr;
		ParametersListSyntax* Params = nullptr;

		inline DelegateDeclarationSyntax(SyntaxNode *const parent)
			: MemberDeclarationSyntax(SyntaxKind::DelegateDeclaration, parent) { }

		inline DelegateDeclarationSyntax(const DelegateDeclarationSyntax& other) = delete;

		inline DelegateDeclarationSyntax(shard::MemberDeclarationInfo& info, SyntaxNode *const parent) : MemberDeclarationSyntax(SyntaxKind::DelegateDeclaration, parent)
		{
			Modifiers = info.Modifiers;
			DelegateToken = info.DeclareType;
			ReturnType = info.ReturnType;
			IdentifierToken = info.Identifier;
			TypeParameters = info.Generics;
		}

		inline virtual ~DelegateDeclarationSyntax()
		{
			if (ReturnType != nullptr)
				delete ReturnType;

			if (Params != nullptr)
				delete Params;
		}
	};
}