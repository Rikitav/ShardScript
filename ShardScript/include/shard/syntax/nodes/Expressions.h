#pragma once
#include <shard/syntax/nodes/ExpressionSyntax.h>
#include <shard/syntax/nodes/ArgumentsListSyntax.h>

#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxToken.h>

#include <memory>
#include <vector>

namespace shard::syntax::nodes
{
	class ConstValueExpressionSyntax : public ExpressionSyntax
	{
	public:
		SyntaxToken Constant;

		ConstValueExpressionSyntax(SyntaxToken constant)
			: ExpressionSyntax(SyntaxKind::ConstExpression), Constant(constant) {
		}
	};

	class MemberAccessExpressionSyntax : public ExpressionSyntax
	{
	public:
		vector<SyntaxToken> Path;

		MemberAccessExpressionSyntax()
			: ExpressionSyntax(SyntaxKind::MemberAccessExpression) {
		}
	};

	class BinaryExpressionSyntax : public ExpressionSyntax
	{
	public:
		SyntaxToken OperatorToken;
		shared_ptr<ExpressionSyntax> Left;
		shared_ptr<ExpressionSyntax> Right;

		BinaryExpressionSyntax(shared_ptr<ExpressionSyntax> left, SyntaxToken operatorToken, shared_ptr<ExpressionSyntax> right)
			: ExpressionSyntax(SyntaxKind::BinaryExpression), Left(left), OperatorToken(operatorToken), Right(right) {
		}
	};

	class InvokationExpressionSyntax : public ExpressionSyntax
	{
	public:
		shared_ptr<MemberAccessExpressionSyntax> MemberAccess;
		shared_ptr<ArgumentsListSyntax> ArgumentsList;

		InvokationExpressionSyntax(shared_ptr<MemberAccessExpressionSyntax> memberAccess, shared_ptr<ArgumentsListSyntax> argumentsList)
			: ExpressionSyntax(SyntaxKind::InvokationExpression), MemberAccess(memberAccess), ArgumentsList(argumentsList) {
		}
	};
}