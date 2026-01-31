#pragma once
#include <shard/SyntaxVisitor.h>
#include <shard/compilation/ProgramVirtualImage.h>
#include <shard/compilation/ByteCodeGenerator.h>

namespace shard
{
	// basically, a compiler
	class AbstractEmmiter : SyntaxVisitor, ByteCodeGenerator
	{
		MethodSymbol* GeneratingFor = nullptr;

	public:
		std::vector<shard::MethodSymbol*> EntryPointCandidates;
		const ProgramVirtualImage& Program;

		inline AbstractEmmiter(ProgramVirtualImage& program, shard::SemanticModel& model, shard::DiagnosticsContext& diagnostics) : SyntaxVisitor(model, diagnostics), Program(program) { }

		void VisitMethodDeclaration(shard::MethodDeclarationSyntax* node) override;
		void VisitConstructorDeclaration(shard::ConstructorDeclarationSyntax* node) override;
		void VisitIndexatorDeclaration(shard::IndexatorDeclarationSyntax* node) override;
		void VisitAccessorDeclaration(shard::AccessorDeclarationSyntax* node) override;

        void VisitExpressionStatement(shard::ExpressionStatementSyntax* node) override;
        void VisitVariableStatement(shard::VariableStatementSyntax* node) override;
        void VisitReturnStatement(shard::ReturnStatementSyntax* node) override;
        void VisitThrowStatement(shard::ThrowStatementSyntax* node) override;
        void VisitBreakStatement(shard::BreakStatementSyntax* node) override;
        void VisitContinueStatement(shard::ContinueStatementSyntax* node) override;

        void VisitWhileStatement(shard::WhileStatementSyntax* node) override;
        void VisitForStatement(shard::ForStatementSyntax* node) override;
        void VisitUntilStatement(shard::UntilStatementSyntax* node) override;

        void VisitConditionalClause(shard::ConditionalClauseBaseSyntax* node) override;
        void VisitIfStatement(shard::IfStatementSyntax* node) override;
        void VisitUnlessStatement(shard::UnlessStatementSyntax* node) override;
        void VisitElseStatement(shard::ElseStatementSyntax* node) override;

        void VisitExpression(shard::ExpressionSyntax* node) override;
        void VisitLiteralExpression(shard::LiteralExpressionSyntax* node) override;
        void VisitBinaryExpression(shard::BinaryExpressionSyntax* node) override;
        void VisitUnaryExpression(shard::UnaryExpressionSyntax* node) override;
        void VisitObjectCreationExpression(shard::ObjectExpressionSyntax* node) override;
        void VisitCollectionExpression(shard::CollectionExpressionSyntax* node) override;
        void VisitLambdaExpression(shard::LambdaExpressionSyntax* node) override;
        void VisitTernaryExpression(shard::TernaryExpressionSyntax* node) override;

        void VisitInvocationExpression(shard::InvokationExpressionSyntax* node) override;
        void VisitMemberAccessExpression(shard::MemberAccessExpressionSyntax* node) override;
        void VisitIndexatorExpression(shard::IndexatorExpressionSyntax* node) override;
	};
}
