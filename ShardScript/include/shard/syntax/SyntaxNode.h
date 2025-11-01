#pragma once
#include <shard/syntax/SyntaxKind.h>

namespace shard::syntax
{
	class SyntaxNode
	{
	public:
		const SyntaxKind Kind;
		SyntaxNode* Parent;
		
		inline SyntaxNode(const SyntaxKind kind, const SyntaxNode* parent)
			: Kind(kind), Parent((SyntaxNode*)parent) { }

		inline SyntaxNode(const SyntaxNode& other)
			: Kind(other.Kind), Parent(other.Parent) { }

		inline virtual ~SyntaxNode()
		{
			Parent = nullptr;
		}

		//virtual vector<SyntaxNode> GetChildNodes();
		//virtual void Accept();
	};
}
