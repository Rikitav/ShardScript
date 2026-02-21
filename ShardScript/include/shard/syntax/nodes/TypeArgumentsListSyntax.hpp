#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/syntax/SyntaxKind.hpp>
#include <shard/syntax/SyntaxNode.hpp>
#include <shard/syntax/SyntaxToken.hpp>

#include <shard/syntax/nodes/TypeSyntax.hpp>

#include <vector>

namespace shard
{
	class SHARD_API TypeArgumentsListSyntax : public SyntaxNode
	{
	public:
		SyntaxToken OpenToken;
		SyntaxToken CloseToken;
		std::vector<TypeSyntax*> Types;

		inline TypeArgumentsListSyntax(SyntaxNode *const parent)
			: SyntaxNode(SyntaxKind::ParametersList, parent) {
		}

		inline TypeArgumentsListSyntax(const TypeArgumentsListSyntax& other) = delete;

		inline virtual ~TypeArgumentsListSyntax()
		{
			for (const TypeSyntax* parameter : Types)
				delete parameter;
		}
	};
}