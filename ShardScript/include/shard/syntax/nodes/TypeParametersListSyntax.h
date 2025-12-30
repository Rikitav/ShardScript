#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxNode.h>
#include <shard/syntax/SyntaxToken.h>

#include <shard/syntax/nodes/TypeSyntax.h>

#include <vector>

namespace shard
{
	class SHARD_API TypeParametersListSyntax : public SyntaxNode
	{
	public:
		SyntaxToken OpenToken;
		SyntaxToken CloseToken;
		std::vector<SyntaxToken> Types;

		inline TypeParametersListSyntax(const SyntaxNode* parent)
			: SyntaxNode(SyntaxKind::ParametersList, parent) { }

		inline TypeParametersListSyntax(const TypeParametersListSyntax& other) = delete;

		inline virtual ~TypeParametersListSyntax()
		{

		}
	};
}