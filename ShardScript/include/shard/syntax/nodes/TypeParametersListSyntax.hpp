#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/syntax/SyntaxKind.hpp>
#include <shard/syntax/SyntaxNode.hpp>
#include <shard/syntax/SyntaxToken.hpp>

#include <shard/syntax/nodes/TypeSyntax.hpp>

#include <vector>

namespace shard
{
	class SHARD_API TypeParametersListSyntax : public SyntaxNode
	{
	public:
		SyntaxToken OpenToken;
		SyntaxToken CloseToken;
		std::vector<SyntaxToken> Types;

		inline TypeParametersListSyntax(SyntaxNode *const parent)
			: SyntaxNode(SyntaxKind::ParametersList, parent) { }

		inline TypeParametersListSyntax(const TypeParametersListSyntax& other) = delete;

		inline virtual ~TypeParametersListSyntax()
		{

		}
	};
}