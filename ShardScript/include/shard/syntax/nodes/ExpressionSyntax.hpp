#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/syntax/SyntaxNode.hpp>
#include <shard/syntax/SyntaxKind.hpp>

namespace shard
{
	class SHARD_API ExpressionSyntax : public SyntaxNode
	{
	public:
		inline ExpressionSyntax(const SyntaxKind kind, SyntaxNode *const parent)
			: SyntaxNode(kind, parent) { }

		inline ExpressionSyntax(const ExpressionSyntax& other) = delete;

		inline virtual ~ExpressionSyntax()
		{

		}
	};
}
