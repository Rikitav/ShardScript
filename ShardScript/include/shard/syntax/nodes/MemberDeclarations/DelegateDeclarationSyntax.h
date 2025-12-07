#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/parsing/lexical/MemberDeclarationInfo.h>

#include <shard/syntax/nodes/MemberDeclarationSyntax.h>
#include <shard/syntax/nodes/ParametersListSyntax.h>

#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxNode.h>
#include <shard/syntax/SyntaxToken.h>

namespace shard::syntax::nodes
{
	class SHARD_API DelegateDeclarationSyntax : public MemberDeclarationSyntax
	{
	public:
		SyntaxToken DelegateToken;
		SyntaxToken Semicolon;
		TypeSyntax* ReturnType = nullptr;
		ParametersListSyntax* Params = nullptr;

		inline DelegateDeclarationSyntax(const SyntaxNode* parent)
			: MemberDeclarationSyntax(SyntaxKind::DelegateDeclaration, parent) { }

		inline DelegateDeclarationSyntax(const DelegateDeclarationSyntax& other) = delete;

		inline DelegateDeclarationSyntax(shard::parsing::lexical::MemberDeclarationInfo& info, const SyntaxNode* parent) : MemberDeclarationSyntax(SyntaxKind::DelegateDeclaration, parent)
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