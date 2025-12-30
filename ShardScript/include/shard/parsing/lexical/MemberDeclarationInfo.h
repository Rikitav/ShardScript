#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/syntax/SyntaxToken.h>
#include <shard/syntax/nodes/TypeParametersListSyntax.h>
#include <shard/syntax/nodes/TypeSyntax.h>

#include <vector>

namespace shard
{
	struct SHARD_API MemberDeclarationInfo
	{
		std::vector<shard::SyntaxToken> Modifiers;
		bool IsCtor = false;
		shard::SyntaxToken DeclareType;
		shard::SyntaxToken Identifier;
		shard::TypeSyntax* ReturnType = nullptr;
		shard::TypeParametersListSyntax* Generics = nullptr;
	};
}
