#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxNode.h>
#include <shard/syntax/SyntaxToken.h>

#include <shard/syntax/nodes/TypeSyntax.h>

#include <vector>

namespace shard
{
	class SHARD_API TypeArgumentsListSyntax : public SyntaxNode
	{
	public:
		SyntaxToken OpenToken;
		SyntaxToken CloseToken;
		std::vector<TypeSyntax*> Types;

		inline TypeArgumentsListSyntax(const SyntaxNode* parent)
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