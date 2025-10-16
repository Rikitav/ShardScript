#pragma once
#include <shard/syntax/nodes/ExpressionSyntax.h>
#include <shard/syntax/nodes/ArgumentsListSyntax.h>
#include <shard/syntax/nodes/IndexatorListSyntax.h>

#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxToken.h>

#include <memory>

namespace shard::syntax::nodes
{
	class ConstValueExpressionSyntax : public ExpressionSyntax
	{
	public:
		SyntaxToken Constant;

		ConstValueExpressionSyntax(SyntaxToken constant)
			: ExpressionSyntax(SyntaxKind::ConstExpression), Constant(constant) { }
	};

	class UnaryExpressionSyntax : public ExpressionSyntax
	{
	public:
		SyntaxToken OperatorToken;
		shared_ptr<ExpressionSyntax> Expression;
		bool IsRightDetermined;

		UnaryExpressionSyntax(SyntaxToken operatorToken, shared_ptr<ExpressionSyntax> expression, bool isRightDetermined)
			: ExpressionSyntax(SyntaxKind::UnaryExpression), OperatorToken(operatorToken), Expression(expression), IsRightDetermined(isRightDetermined) { }
	};

	class BinaryExpressionSyntax : public ExpressionSyntax
	{
	public:
		SyntaxToken OperatorToken;
		shared_ptr<ExpressionSyntax> Left;
		shared_ptr<ExpressionSyntax> Right;

		BinaryExpressionSyntax(shared_ptr<ExpressionSyntax> left, SyntaxToken operatorToken, shared_ptr<ExpressionSyntax> right)
			: ExpressionSyntax(SyntaxKind::BinaryExpression), Left(left), OperatorToken(operatorToken), Right(right) { }
	};

	class MemberAccessExpressionSyntax : public ExpressionSyntax
	{
	public:
		SyntaxToken IdentifierToken;
		SyntaxToken DelimeterToken;
		shared_ptr<MemberAccessExpressionSyntax> NextAccess;

		MemberAccessExpressionSyntax(SyntaxKind kind)
			: ExpressionSyntax(kind) { }
	};

	class FieldAccesExpressionSyntax : public MemberAccessExpressionSyntax
	{
	public:
		FieldAccesExpressionSyntax()
			: MemberAccessExpressionSyntax(SyntaxKind::FieldAccessExpression) { }
	};

	class InvokationExpressionSyntax : public MemberAccessExpressionSyntax
	{
	public:
		shared_ptr<ArgumentsListSyntax> ArgumentsList;

		InvokationExpressionSyntax()
			: MemberAccessExpressionSyntax(SyntaxKind::InvokationExpression) { }
	};

	class IndexatorExpressionSyntax : public MemberAccessExpressionSyntax
	{
	public:
		shared_ptr<IndexatorListSyntax> IndexatorList;

		IndexatorExpressionSyntax()
			: MemberAccessExpressionSyntax(SyntaxKind::IndexatorExpression) { }
	};
}