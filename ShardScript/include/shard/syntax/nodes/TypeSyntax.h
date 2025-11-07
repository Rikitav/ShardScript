#pragma once
#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxNode.h>
#include <string>

namespace shard::syntax::nodes
{
	class TypeSyntax : public SyntaxNode
	{
	public:
		inline TypeSyntax(const SyntaxKind kind, const SyntaxNode* parent)
			: SyntaxNode(kind, parent) { }

		inline virtual std::wstring ToString()
		{
			return L"<unknown>";
		}
	};
}