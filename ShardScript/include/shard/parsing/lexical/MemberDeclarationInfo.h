#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/syntax/SyntaxToken.h>
#include <shard/syntax/nodes/TypeParametersListSyntax.h>
#include <shard/syntax/nodes/TypeSyntax.h>

#include <vector>

namespace shard::parsing::lexical
{
	struct SHARD_API MemberDeclarationInfo
	{
		std::vector<shard::syntax::SyntaxToken> Modifiers;
		bool IsCtor = false;
		shard::syntax::SyntaxToken DeclareType;
		shard::syntax::SyntaxToken Identifier;
		shard::syntax::nodes::TypeSyntax* ReturnType = nullptr;
		shard::syntax::nodes::TypeParametersListSyntax* Generics = nullptr;
	};
}
