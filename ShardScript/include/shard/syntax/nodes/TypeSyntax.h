#pragma once
#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxNode.h>
#include <shard/syntax/symbols/TypeSymbol.h>
#include <string>

namespace shard::syntax::nodes
{
	class TypeSyntax : public SyntaxNode
	{
	public:
		shard::syntax::symbols::TypeSymbol* Symbol = nullptr;

		inline TypeSyntax(const SyntaxKind kind, const SyntaxNode* parent)
			: SyntaxNode(kind, parent) { }

		inline virtual std::wstring ToString()
		{
			return L"<unknown>";
		}
	};
}