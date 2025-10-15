#pragma once
#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxToken.h>

#include <shard/syntax/nodes/MemberDeclarationSyntax.h>
#include <shard/syntax/nodes/ParametersListSyntax.h>
#include <shard/syntax/nodes/StatementsBlockSyntax.h>

#include <shard/parsing/structures/MemberDeclarationInfo.h>

#include <memory>
#include <vector>

using namespace std;
using namespace shard::parsing::structures;

namespace shard::syntax::nodes
{
	class MethodDeclarationSyntax : public MemberDeclarationSyntax
	{
	public:
		SyntaxToken ReturnType;
		shared_ptr<StatementsBlockSyntax> Body = nullptr;
		shared_ptr<ParametersListSyntax> Params = nullptr;

		MethodDeclarationSyntax() : MemberDeclarationSyntax(SyntaxKind::MethodDeclaration), Body(nullptr), Params(nullptr)
		{

		}

		MethodDeclarationSyntax(MemberDeclarationInfo& info) : MemberDeclarationSyntax(SyntaxKind::ClassDeclaration), Body(nullptr), Params(nullptr)
		{
			Modifiers = info.Modifiers;
			Identifier = info.Identifier;
			ReturnType = info.ReturnType;
		}
	};
}
