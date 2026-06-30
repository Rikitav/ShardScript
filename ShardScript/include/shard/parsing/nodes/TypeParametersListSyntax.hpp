#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxNode.hpp>
#include <shard/parsing/SyntaxToken.hpp>

#include <shard/parsing/nodes/TypeSyntax.hpp>

#include <vector>

namespace shard
{
	class SHARD_API TypeParametersListSyntax : public SyntaxNode
	{
	public:
		SyntaxToken OpenToken;
		SyntaxToken CloseToken;
		std::vector<SyntaxToken> Types;

		inline TypeParametersListSyntax(SyntaxNode* parent)
			: SyntaxNode(SyntaxKind::ParametersList, parent) { }

		inline TypeParametersListSyntax(const TypeParametersListSyntax& other) = delete;

		inline virtual ~TypeParametersListSyntax()
		{

		}
	};
}