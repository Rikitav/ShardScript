#pragma once
#include <shard/syntax/SyntaxNode.h>

namespace shard::syntax::nodes
{
	class StatementsBlockSyntax : public SyntaxNode
	{
	public:
		SyntaxToken OpenBraceToken;
		SyntaxToken CloseBraceToken;
		vector<shared_ptr<StatementSyntax>> Statements;

		StatementsBlockSyntax() : SyntaxNode(SyntaxKind::StatementsBlock)
		{

		}
	};
}
