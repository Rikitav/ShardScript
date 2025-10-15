#pragma once
#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxNode.h>
#include <shard/syntax/SyntaxToken.h>
#include <shard/syntax/nodes/ExpressionSyntax.h>
#include <memory>
#include <vector>

namespace shard::syntax::nodes
{
	class IndexatorListSyntax : public SyntaxNode
	{
	public:
		SyntaxToken OpenSquareToken;
		vector<shared_ptr<ExpressionSyntax>> Arguments;
		SyntaxToken CloseSquareToken;

		IndexatorListSyntax()
			: SyntaxNode(SyntaxKind::IndexatorList) { }
	};
}
