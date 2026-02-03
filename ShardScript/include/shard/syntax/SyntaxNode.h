#pragma once
#include <shard/ShardScriptAPI.h>
#include <shard/syntax/SyntaxKind.h>

namespace shard
{
	class SHARD_API SyntaxNode
	{
	public:
		const SyntaxKind Kind;
		SyntaxNode *const Parent;
		
		inline SyntaxNode(const SyntaxKind kind, SyntaxNode *const parent)
			: Kind(kind), Parent(parent) { }

		inline SyntaxNode(const SyntaxNode& other) = delete;

		inline virtual ~SyntaxNode()
		{
			*const_cast<SyntaxNode**>(&Parent) = nullptr;
		}
	};
}
