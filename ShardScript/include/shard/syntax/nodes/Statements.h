#pragma once
#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxToken.h>

#include <shard/syntax/nodes/ExpressionSyntax.h>
#include <shard/syntax/nodes/StatementSyntax.h>

#include <memory>

namespace shard::syntax::nodes
{
	class KeywordStatementSyntax : public StatementSyntax
	{
	public:
		SyntaxToken Keyword;

		KeywordStatementSyntax(SyntaxToken keyword)
			: StatementSyntax(SyntaxKind::KeywordStatement), Keyword(keyword) {
		}
	};

	class ExpressionStatementSyntax : public StatementSyntax
	{
	public:
		shared_ptr<ExpressionSyntax> Expression;

		ExpressionStatementSyntax()
			: StatementSyntax(SyntaxKind::ExpressionStatement) {
		}

		ExpressionStatementSyntax(shared_ptr<ExpressionSyntax> expression)
			: StatementSyntax(SyntaxKind::ExpressionStatement), Expression(expression) {
		}
	};

	class VariableStatementSyntax : public StatementSyntax
	{
	public:
		SyntaxToken Type;
		SyntaxToken Name;
		SyntaxToken AssignOperator;
		shared_ptr<ExpressionSyntax> Expression;

		VariableStatementSyntax(SyntaxToken type, SyntaxToken name, SyntaxToken assignOperator, shared_ptr<ExpressionSyntax> expression)
			: StatementSyntax(SyntaxKind::VariableStatement), Type(type), Name(name), AssignOperator(assignOperator), Expression(expression) {
		}
	};

	/*
	class BlockStatement : public StatementSyntax
	{
		shared_ptr<BlockDeclarationSyntax>& Block;
	};
	*/
}
