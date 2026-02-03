#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/syntax/SyntaxNode.h>
#include <shard/syntax/SyntaxKind.h>

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
