#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/syntax/SyntaxToken.hpp>
#include <shard/syntax/nodes/TypeParametersListSyntax.hpp>
#include <shard/syntax/nodes/TypeSyntax.hpp>
#include <shard/syntax/nodes/AttributeSyntax.hpp>

#include <vector>
#include <memory>

namespace shard
{
	struct SHARD_API MemberDeclarationInfo
	{
		std::vector<std::unique_ptr<shard::AttributeSyntax>> Attributes;
		std::vector<shard::SyntaxToken> Modifiers;
		shard::SyntaxToken DeclareType;
		shard::SyntaxToken Identifier;
		std::unique_ptr<shard::TypeSyntax> ReturnType = nullptr;
		std::unique_ptr<shard::TypeParametersListSyntax> Generics = nullptr;
	};
}
