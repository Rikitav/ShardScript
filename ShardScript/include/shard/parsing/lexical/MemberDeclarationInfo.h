#pragma once
#include <shard/syntax/SyntaxToken.h>
#include <shard/syntax/nodes/ParametersListSyntax.h>
#include <shard/syntax/nodes/TypeSyntax.h>
#include <vector>

namespace shard::parsing::lexical
{
	struct MemberDeclarationInfo
	{
		std::vector<shard::syntax::SyntaxToken> Modifiers;
		bool IsCtor = false;
		shard::syntax::SyntaxToken DeclareType;
		shard::syntax::SyntaxToken Identifier;
		shard::syntax::nodes::TypeSyntax* ReturnType = nullptr;
	};
}
