#pragma once
#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/nodes/Expressions/LinkedExpressionSyntax.h>
#include <string>

namespace shard::syntax::nodes
{
	class TypeExpressionSyntax : public LinkedExpressionNode
	{
	public:
		inline TypeExpressionSyntax(const SyntaxKind kind, const LinkedExpressionNode* previous, const SyntaxNode* parent)
			: LinkedExpressionNode(kind, previous, parent) { }

		inline virtual ~TypeExpressionSyntax() = default;

		inline virtual std::wstring ToString()
		{
			return L"<unknown>";
		}
	};
}