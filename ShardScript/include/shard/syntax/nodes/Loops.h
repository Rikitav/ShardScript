#pragma once
#include <shard/syntax/nodes/StatementSyntax.h>
#include <shard/syntax/nodes/ExpressionSyntax.h>
#include <shard/syntax/nodes/Statements.h>
#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxToken.h>
#include <memory>

namespace shard::syntax::nodes
{
	class ForStatementSyntax : public KeywordStatementSyntax
	{
	public:
		SyntaxToken OpenCurlToken;
		shared_ptr<StatementSyntax> InitializerStatement = nullptr;
		SyntaxToken FirstSemicolon;
		shared_ptr<ExpressionSyntax> ConditionExpression = nullptr;
		SyntaxToken SecondSemicolon;
		shared_ptr<StatementSyntax> AfterRepeatStatement = nullptr;
		SyntaxToken CloseCurlToken;

		ForStatementSyntax()
			: KeywordStatementSyntax(SyntaxKind::ForStatementSyntax) { }
	};
}
