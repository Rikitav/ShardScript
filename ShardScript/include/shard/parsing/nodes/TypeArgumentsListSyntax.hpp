#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxNode.hpp>
#include <shard/parsing/SyntaxToken.hpp>

#include <shard/parsing/nodes/TypeSyntax.hpp>

#include <vector>
#include <memory>

namespace shard
{
	class SHARD_API TypeArgumentsListSyntax : public SyntaxNode
	{
	public:
		SyntaxToken OpenToken;
		SyntaxToken CloseToken;
		std::vector<std::unique_ptr<TypeSyntax>> Types;

		inline TypeArgumentsListSyntax(SyntaxNode* parent)
			: SyntaxNode(SyntaxKind::ParametersList, parent) {
		}

		inline TypeArgumentsListSyntax(const TypeArgumentsListSyntax& other) = delete;

		inline virtual ~TypeArgumentsListSyntax() = default;
	};
}
