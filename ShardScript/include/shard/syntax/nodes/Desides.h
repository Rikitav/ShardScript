#pragma once
#include <shard/syntax/nodes/Statements.h>

namespace shard::syntax::nodes
{
	class IfStatementSyntax : public ConditionalStatementSyntax
	{
	public:
		IfStatementSyntax()
			: ConditionalStatementSyntax(SyntaxKind::IfStatement) {
		}
	};

	class UnlessStatementSyntax : public ConditionalStatementSyntax
	{
	public:
		UnlessStatementSyntax()
			: ConditionalStatementSyntax(SyntaxKind::UnlessStatement) {
		}
	};

	/*
	class ElseIfStatementSyntax : public ConditionalStatementSyntax
	{
		SyntaxToken IfKeyword;

	public:
		ElseIfStatementSyntax()
			: ConditionalStatementSyntax(SyntaxKind::ElseIfStatement) {
		}
	};

	class ElseUnlessStatementSyntax : public ConditionalStatementSyntax
	{
		SyntaxToken UnlessKeyword;

	public:
		ElseUnlessStatementSyntax()
			: ConditionalStatementSyntax(SyntaxKind::ElseUnlessStatement) {
		}
	};
	*/

	class ElseSatetmentSyntax : public DesideStatementSyntax
	{
	public:
		ElseSatetmentSyntax()
			: DesideStatementSyntax(SyntaxKind::ElseStatement) {
		}
	};
}