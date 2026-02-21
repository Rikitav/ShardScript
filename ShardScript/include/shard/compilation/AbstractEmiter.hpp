#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/compilation/ProgramVirtualImage.hpp>
#include <shard/compilation/ByteCodeEncoder.hpp>

#include <shard/parsing/analysis/DiagnosticsContext.hpp>
#include <shard/parsing/semantic/SemanticModel.hpp>
#include <shard/parsing/SyntaxTree.hpp>

#include <shard/SyntaxVisitor.hpp>

#include <shard/syntax/nodes/ArgumentsListSyntax.hpp>

#include <shard/syntax/nodes/Expressions/BinaryExpressionSyntax.hpp>
#include <shard/syntax/nodes/Expressions/CollectionExpressionSyntax.hpp>
#include <shard/syntax/nodes/Expressions/LambdaExpressionSyntax.hpp>
#include <shard/syntax/nodes/Expressions/LinkedExpressionSyntax.hpp>
#include <shard/syntax/nodes/Expressions/LiteralExpressionSyntax.hpp>
#include <shard/syntax/nodes/Expressions/ObjectExpressionSyntax.hpp>
#include <shard/syntax/nodes/Expressions/TernaryExpressionSyntax.hpp>
#include <shard/syntax/nodes/Expressions/UnaryExpressionSyntax.hpp>

#include <shard/syntax/nodes/Loops/ForStatementSyntax.hpp>
#include <shard/syntax/nodes/Loops/UntilStatementSyntax.hpp>
#include <shard/syntax/nodes/Loops/WhileStatementSyntax.hpp>

#include <shard/syntax/nodes/MemberDeclarations/AccessorDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/ConstructorDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/MethodDeclarationSyntax.hpp>

#include <shard/syntax/nodes/Statements/BreakStatementSyntax.hpp>
#include <shard/syntax/nodes/Statements/ConditionalClauseSyntax.hpp>
#include <shard/syntax/nodes/Statements/ContinueStatementSyntax.hpp>
#include <shard/syntax/nodes/Statements/ExpressionStatementSyntax.hpp>
#include <shard/syntax/nodes/Statements/ReturnStatementSyntax.hpp>
#include <shard/syntax/nodes/Statements/ThrowStatementSyntax.hpp>
#include <shard/syntax/nodes/Statements/VariableStatementSyntax.hpp>

#include <shard/syntax/symbols/MethodSymbol.hpp>

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
