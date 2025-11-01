#pragma once
#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxToken.h>
#include <shard/syntax/SyntaxNode.h>

#include <shard/syntax/nodes/MemberDeclarationSyntax.h>
#include <shard/syntax/nodes/ParametersListSyntax.h>
#include <shard/syntax/nodes/StatementsBlockSyntax.h>
#include <shard/syntax/nodes/TypeSyntax.h>

#include <shard/parsing/lexical/MemberDeclarationInfo.h>
#include <vector>

namespace shard::syntax::nodes
{
	class MethodDeclarationSyntax : public MemberDeclarationSyntax
	{
	public:
		StatementsBlockSyntax* Body = nullptr;
		ParametersListSyntax* Params = nullptr;
		TypeSyntax* ReturnType = nullptr;

		inline MethodDeclarationSyntax(shard::parsing::lexical::MemberDeclarationInfo& info, const SyntaxNode* parent) : MemberDeclarationSyntax(SyntaxKind::MethodDeclaration, parent)
		{
			Modifiers = info.Modifiers;
			IdentifierToken = info.Identifier;
			ReturnType = info.ReturnType;
			Params = info.Params;
		}
	};
}
