#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/compilation/ProgramVirtualImage.h>
#include <shard/compilation/ByteCodeEncoder.h>

#include <shard/parsing/analysis/DiagnosticsContext.h>
#include <shard/parsing/semantic/SemanticModel.h>
#include <shard/parsing/SyntaxTree.h>

#include <shard/SyntaxVisitor.h>

#include <shard/syntax/nodes/ArgumentsListSyntax.h>

#include <shard/syntax/nodes/Expressions/BinaryExpressionSyntax.h>
#include <shard/syntax/nodes/Expressions/CollectionExpressionSyntax.h>
#include <shard/syntax/nodes/Expressions/LambdaExpressionSyntax.h>
#include <shard/syntax/nodes/Expressions/LinkedExpressionSyntax.h>
#include <shard/syntax/nodes/Expressions/LiteralExpressionSyntax.h>
#include <shard/syntax/nodes/Expressions/ObjectExpressionSyntax.h>
#include <shard/syntax/nodes/Expressions/TernaryExpressionSyntax.h>
#include <shard/syntax/nodes/Expressions/UnaryExpressionSyntax.h>

#include <shard/syntax/nodes/Loops/ForStatementSyntax.h>
#include <shard/syntax/nodes/Loops/UntilStatementSyntax.h>
#include <shard/syntax/nodes/Loops/WhileStatementSyntax.h>

#include <shard/syntax/nodes/MemberDeclarations/AccessorDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/ConstructorDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/MethodDeclarationSyntax.h>

#include <shard/syntax/nodes/Statements/BreakStatementSyntax.h>
#include <shard/syntax/nodes/Statements/ConditionalClauseSyntax.h>
#include <shard/syntax/nodes/Statements/ContinueStatementSyntax.h>
#include <shard/syntax/nodes/Statements/ExpressionStatementSyntax.h>
#include <shard/syntax/nodes/Statements/ReturnStatementSyntax.h>
#include <shard/syntax/nodes/Statements/ThrowStatementSyntax.h>
#include <shard/syntax/nodes/Statements/VariableStatementSyntax.h>

#include <shard/syntax/symbols/MethodSymbol.h>

#include <vector>

namespace shard
{
	// basically, a compiler
	class SHARD_API AbstractEmiter : SyntaxVisitor
	{
        shard::ByteCodeEncoder Encoder;
        shard::MethodSymbol* GeneratingFor = nullptr;
		shard::ProgramVirtualImage& Program;
		std::vector<shard::MethodSymbol*> EntryPointCandidates;

	public:
		inline AbstractEmiter(shard::ProgramVirtualImage& program, shard::SemanticModel& model, shard::DiagnosticsContext& diagnostics)
            : SyntaxVisitor(model, diagnostics), Program(program), Encoder() { }

        void VisitSyntaxTree(shard::SyntaxTree& tree) override;

        void VisitArgumentsList(ArgumentsListSyntax* node) override;

		void VisitMethodDeclaration(shard::MethodDeclarationSyntax *const node) override;
		void VisitConstructorDeclaration(shard::ConstructorDeclarationSyntax *const node) override;
		void VisitAccessorDeclaration(shard::AccessorDeclarationSyntax *const node) override;

        void VisitExpressionStatement(shard::ExpressionStatementSyntax *const node) override;
        void VisitVariableStatement(shard::VariableStatementSyntax *const node) override;
        void VisitReturnStatement(shard::ReturnStatementSyntax *const node) override;
        void VisitThrowStatement(shard::ThrowStatementSyntax *const node) override;
        void VisitBreakStatement(shard::BreakStatementSyntax *const node) override;
        void VisitContinueStatement(shard::ContinueStatementSyntax *const node) override;

        void VisitWhileStatement(shard::WhileStatementSyntax *const node) override;
        void VisitForStatement(shard::ForStatementSyntax *const node) override;
        void VisitUntilStatement(shard::UntilStatementSyntax *const node) override;

        void VisitConditionalClause(shard::ConditionalClauseBaseSyntax *const node) override;
        void VisitIfStatement(shard::IfStatementSyntax *const node) override;
        void VisitUnlessStatement(shard::UnlessStatementSyntax *const node) override;
        void VisitElseStatement(shard::ElseStatementSyntax *const node) override;

        void VisitLiteralExpression(shard::LiteralExpressionSyntax *const node) override;
        void VisitBinaryExpression(shard::BinaryExpressionSyntax *const node) override;
        void VisitUnaryExpression(shard::UnaryExpressionSyntax *const node) override;
        void VisitObjectCreationExpression(shard::ObjectExpressionSyntax *const node) override;
        void VisitCollectionExpression(shard::CollectionExpressionSyntax *const node) override;
        void VisitLambdaExpression(shard::LambdaExpressionSyntax *const node) override;
        void VisitTernaryExpression(shard::TernaryExpressionSyntax *const node) override;

        void VisitInvocationExpression(shard::InvokationExpressionSyntax *const node) override;
        void VisitMemberAccessExpression(shard::MemberAccessExpressionSyntax *const node) override;
        void VisitIndexatorExpression(shard::IndexatorExpressionSyntax *const node) override;
	};
}
