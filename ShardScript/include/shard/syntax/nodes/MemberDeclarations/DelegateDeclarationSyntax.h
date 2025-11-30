#pragma once
#include <shard/parsing/lexical/MemberDeclarationInfo.h>

#include <shard/syntax/nodes/MemberDeclarationSyntax.h>
#include <shard/syntax/nodes/ParametersListSyntax.h>

#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxNode.h>
#include <shard/syntax/SyntaxToken.h>

namespace shard::syntax::nodes
{
	class DelegateDeclarationSyntax : public MemberDeclarationSyntax
	{
	public:
		SyntaxToken DelegateToken;
		SyntaxToken Semicolon;
		TypeSyntax* ReturnType = nullptr;
		ParametersListSyntax* Params = nullptr;

		inline DelegateDeclarationSyntax(const SyntaxNode* parent)
			: MemberDeclarationSyntax(SyntaxKind::DelegateDeclaration, parent) { }

		inline DelegateDeclarationSyntax(const DelegateDeclarationSyntax& other)
			: MemberDeclarationSyntax(other), DelegateToken(other.DelegateToken), Semicolon(other.Semicolon), Params(other.Params) { }

		inline DelegateDeclarationSyntax(shard::parsing::lexical::MemberDeclarationInfo& info, const SyntaxNode* parent) : MemberDeclarationSyntax(SyntaxKind::DelegateDeclaration, parent)
		{
			Modifiers = info.Modifiers;
			DelegateToken = info.DeclareType;
			ReturnType = info.ReturnType;
			IdentifierToken = info.Identifier;
		}

		inline virtual ~DelegateDeclarationSyntax()
		{
			if (Params != nullptr)
				delete Params;
		}
	};
}