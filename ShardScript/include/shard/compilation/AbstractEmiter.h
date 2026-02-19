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
#include <stack>

namespace shard
{
	// basically, a compiler
	class SHARD_API AbstractEmiter : public SyntaxVisitor
	{
        struct LoopScope
        {
            size_t LoopStart; // Address of first OpCode of loop, used for 'looping jump'
            size_t BlockEnd; // Addredd of OpCode right after last OpCode of looping block, used for 'continue' statement
            size_t LoopEnd; // Address of OpCode right after last OpCode of entire loop, used for 'looping exit', or 'break' statement
            
            std::vector<size_t> BlockEndBacktracks;
            std::vector<size_t> LoopEndBacktracks;
        };

        struct ClauseScope
        {
            size_t ClauseEnd; // Address of OpCode right after last OpCode of entire clause

            std::vector<size_t> ClauseEndBacktracks;
        };

        ByteCodeEncoder Encoder;
        MethodSymbol* GeneratingFor = nullptr;
		ProgramVirtualImage& Program;
		std::vector<MethodSymbol*> EntryPointCandidates;

        std::stack<LoopScope> Loops;
        std::stack<ClauseScope> Clauses;

	public:
		inline AbstractEmiter(ProgramVirtualImage& program, SemanticModel& model, DiagnosticsContext& diagnostics)
            : SyntaxVisitor(model, diagnostics), Program(program), Encoder() { }
        
        void SetEntryPoint();
        void SetGeneratingTarget(MethodSymbol* method);
        void VisitSyntaxTree(SyntaxTree& tree) override;

        void VisitArgumentsList(ArgumentsListSyntax* node) override;

		void VisitMethodDeclaration(MethodDeclarationSyntax *const node) override;
		void VisitConstructorDeclaration(ConstructorDeclarationSyntax *const node) override;
		void VisitAccessorDeclaration(AccessorDeclarationSyntax *const node) override;

        void VisitExpressionStatement(ExpressionStatementSyntax *const node) override;
        void VisitVariableStatement(VariableStatementSyntax *const node) override;
        void VisitReturnStatement(ReturnStatementSyntax *const node) override;
        void VisitThrowStatement(ThrowStatementSyntax *const node) override;
        void VisitBreakStatement(BreakStatementSyntax *const node) override;
        void VisitContinueStatement(ContinueStatementSyntax *const node) override;

        void VisitWhileStatement(WhileStatementSyntax *const node) override;
        void VisitForStatement(ForStatementSyntax *const node) override;
        void VisitUntilStatement(UntilStatementSyntax *const node) override;

        void VisitIfStatement(IfStatementSyntax *const node) override;
        void VisitUnlessStatement(UnlessStatementSyntax *const node) override;
        void VisitElseStatement(ElseStatementSyntax *const node) override;

        void VisitLiteralExpression(LiteralExpressionSyntax *const node) override;
        void VisitObjectCreationExpression(ObjectExpressionSyntax *const node) override;
        void VisitCollectionExpression(CollectionExpressionSyntax *const node) override;
        void VisitLambdaExpression(LambdaExpressionSyntax *const node) override;
        void VisitTernaryExpression(TernaryExpressionSyntax *const node) override;

        void VisitUnaryExpression(UnaryExpressionSyntax* const node) override;
        void VisitUnaryAssignExpression(UnaryExpressionSyntax* const node);

        void VisitBinaryExpression(BinaryExpressionSyntax *const node) override;
        void VisitBinaryAssignExpression(BinaryExpressionSyntax* const node);

        void VisitInvocationExpression(InvokationExpressionSyntax *const node) override;
        void VisitIndexatorExpression(IndexatorExpressionSyntax *const node) override;
        void VisitMemberAccessExpression(MemberAccessExpressionSyntax *const node) override;
	};
}
