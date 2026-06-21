#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/syntax/SyntaxKind.hpp>
#include <shard/syntax/SyntaxNode.hpp>
#include <shard/syntax/SyntaxToken.hpp>

#include <shard/syntax/nodes/TypeSyntax.hpp>

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
