#pragma once
#include <shard/ShardScriptAPI.h>
#include <shard/syntax/SyntaxKind.h>

namespace shard
{
	class SHARD_API SyntaxNode
	{
	public:
		const SyntaxKind Kind;
		SyntaxNode* Parent;
		
		inline SyntaxNode(const SyntaxKind kind, const SyntaxNode* parent)
			: Kind(kind), Parent((SyntaxNode*)parent) { }

		inline SyntaxNode(const SyntaxNode& other) = delete;

		inline virtual ~SyntaxNode()
		{
			Parent = nullptr;
		}
	};
}
