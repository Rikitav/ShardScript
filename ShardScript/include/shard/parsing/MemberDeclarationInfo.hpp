#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/parsing/SyntaxToken.hpp>
#include <shard/parsing/nodes/TypeParametersListSyntax.hpp>
#include <shard/parsing/nodes/TypeSyntax.hpp>
#include <shard/parsing/nodes/AttributeSyntax.hpp>

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
