#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/syntax/SyntaxToken.hpp>
#include <shard/syntax/nodes/TypeParametersListSyntax.hpp>
#include <shard/syntax/nodes/TypeSyntax.hpp>

#include <vector>

namespace shard
{
	struct SHARD_API MemberDeclarationInfo
	{
		std::vector<shard::SyntaxToken> Modifiers;
		shard::SyntaxToken DeclareType;
		shard::SyntaxToken Identifier;
		shard::TypeSyntax* ReturnType = nullptr;
		shard::TypeParametersListSyntax* Generics = nullptr;
	};
}
