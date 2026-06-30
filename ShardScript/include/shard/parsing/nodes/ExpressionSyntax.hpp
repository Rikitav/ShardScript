#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/parsing/SyntaxNode.hpp>
#include <shard/parsing/SyntaxKind.hpp>

namespace shard
{
	class SHARD_API ExpressionSyntax : public SyntaxNode
	{
	public:
		inline ExpressionSyntax(const SyntaxKind kind, SyntaxNode* parent)
			: SyntaxNode(kind, parent) { }

		inline ExpressionSyntax(const ExpressionSyntax& other) = delete;

		inline virtual ~ExpressionSyntax()
		{

		}
	};
}
