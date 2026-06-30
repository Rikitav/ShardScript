#pragma once
#include <shard/ShardScriptAPI.hpp>
#include <shard/parsing/SyntaxKind.hpp>

namespace shard
{
	class SHARD_API SyntaxNode
	{
	public:
		const SyntaxKind Kind;
		SyntaxNode* Parent;
		
		inline SyntaxNode(const SyntaxKind kind, SyntaxNode* parent)
			: Kind(kind), Parent(parent) { }

		inline SyntaxNode(const SyntaxNode& other) = delete;

		inline virtual ~SyntaxNode()
		{
			Parent = nullptr;
		}
	};
}
