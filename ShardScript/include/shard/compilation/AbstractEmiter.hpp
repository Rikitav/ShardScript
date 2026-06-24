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
#include <shard/syntax/nodes/Expressions/RangeExpressionSyntax.hpp>
#include <shard/syntax/nodes/Expressions/LambdaExpressionSyntax.hpp>
#include <shard/syntax/nodes/Expressions/LinkedExpressionSyntax.hpp>
#include <shard/syntax/nodes/Expressions/LiteralExpressionSyntax.hpp>
#include <shard/syntax/nodes/Expressions/ObjectExpressionSyntax.hpp>
#include <shard/syntax/nodes/Expressions/TernaryExpressionSyntax.hpp>
#include <shard/syntax/nodes/Expressions/UnaryExpressionSyntax.hpp>
#include <shard/syntax/nodes/Expressions/CastExpressionSyntax.hpp>
#include <shard/syntax/nodes/Expressions/IsExpressionSyntax.hpp>

#include <shard/syntax/nodes/Loops/ForStatementSyntax.hpp>
#include <shard/syntax/nodes/Loops/ForEachStatementSyntax.hpp>
#include <shard/syntax/nodes/Loops/UntilStatementSyntax.hpp>
#include <shard/syntax/nodes/Loops/WhileStatementSyntax.hpp>

#include <shard/syntax/nodes/MemberDeclarations/AccessorDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/ConstructorDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/MethodDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/OperatorDeclarationSyntax.hpp>

#include <shard/syntax/nodes/Statements/BreakStatementSyntax.hpp>
#include <shard/syntax/nodes/Statements/ConditionalClauseSyntax.hpp>
#include <shard/syntax/nodes/Statements/ContinueStatementSyntax.hpp>
#include <shard/syntax/nodes/Statements/ExpressionStatementSyntax.hpp>
#include <shard/syntax/nodes/Statements/DeferStatementSyntax.hpp>
#include <shard/syntax/nodes/Statements/ReturnStatementSyntax.hpp>
#include <shard/syntax/nodes/Statements/ThrowStatementSyntax.hpp>
#include <shard/syntax/nodes/Statements/TryStatementSyntax.hpp>
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
            std::size_t LoopStart = 0;   // Address of first OpCode of loop, used for 'looping jump'
            std::size_t BlockEnd = 0;    // Addredd of OpCode right after last OpCode of looping block, used for 'continue' statement
            std::size_t LoopEnd = 0;     // Address of OpCode right after last OpCode of entire loop, used for 'looping exit', or 'break' statement
            
            std::vector<std::size_t> BlockEndBacktracks;
            std::vector<std::size_t> LoopEndBacktracks;
        };

        struct ClauseScope
        {
            std::size_t ClauseEnd = 0; // Address of OpCode right after last OpCode of entire clause

            std::vector<std::size_t> ClauseEndBacktracks;
        };

        ByteCodeEncoder Encoder;
        MethodSymbol* GeneratingFor = nullptr;
		ProgramVirtualImage& Program;
		std::vector<MethodSymbol*> EntryPointCandidates;

		bool PopExpressionStatement = true;
        std::stack<LoopScope> Loops;
        std::stack<ClauseScope> Clauses;

        struct DeferScope
        {
            std::vector<DeferStatementSyntax*> Defers;
            bool IsLoop = false;
        };
        std::vector<DeferScope> DeferScopes;
        bool PendingDeferScopeIsLoop = false;

        void EmitDefer(DeferStatementSyntax* defer);
        void EmitCurrentScopeDefers();
        void EmitDefersUntilLoop();
        void EmitAllDefers();

	public:
		inline AbstractEmiter(ProgramVirtualImage& program, SemanticModel& model, DiagnosticsContext& diagnostics)
            : SyntaxVisitor(model, diagnostics), Program(program), Encoder() { }
        
        void SetEntryPoint();
        void SetGeneratingTarget(MethodSymbol* method);
		void SetPopExpressionStatement(bool pop)
		{
			PopExpressionStatement = pop;
		}

        void VisitSyntaxTree(SyntaxTree& tree) override;

        void VisitArgumentsList(ArgumentsListSyntax* node) override;

		void VisitMethodDeclaration(MethodDeclarationSyntax* node) override;
		void VisitOperatorDeclaration(OperatorDeclarationSyntax* node) override;
		void VisitConstructorDeclaration(ConstructorDeclarationSyntax* node) override;
		void VisitAccessorDeclaration(AccessorDeclarationSyntax* node) override;

        void VisitExpressionStatement(ExpressionStatementSyntax* node) override;
        void VisitVariableStatement(VariableStatementSyntax* node) override;
        void VisitReturnStatement(ReturnStatementSyntax* node) override;
        void VisitThrowStatement(ThrowStatementSyntax* node) override;
        void VisitTryStatement(TryStatementSyntax* node) override;
        void VisitBreakStatement(BreakStatementSyntax* node) override;
        void VisitContinueStatement(ContinueStatementSyntax* node) override;
        void VisitDeferStatement(DeferStatementSyntax* node) override;
        void VisitStatementsBlock(StatementsBlockSyntax* node) override;

        void VisitWhileStatement(WhileStatementSyntax* node) override;
        void VisitForStatement(ForStatementSyntax* node) override;
        void VisitForEachStatement(ForEachStatementSyntax* node) override;
        void VisitUntilStatement(UntilStatementSyntax* node) override;

        void VisitIfStatement(IfStatementSyntax* node) override;
        void VisitUnlessStatement(UnlessStatementSyntax* node) override;
        void VisitElseStatement(ElseStatementSyntax* node) override;

        void VisitLiteralExpression(LiteralExpressionSyntax* node) override;
        void VisitObjectCreationExpression(ObjectExpressionSyntax* node) override;
        void VisitCollectionExpression(CollectionExpressionSyntax* node) override;
        void VisitRangeExpression(RangeExpressionSyntax* node) override;
        void VisitLambdaExpression(LambdaExpressionSyntax* node) override;
        void VisitTernaryExpression(TernaryExpressionSyntax* node) override;

        void VisitUnaryExpression(UnaryExpressionSyntax* node) override;
        void VisitUnaryAssignExpression(UnaryExpressionSyntax* node);

        void VisitBinaryExpression(BinaryExpressionSyntax* node) override;
        void VisitBinaryAssignExpression(BinaryExpressionSyntax* node);

        void VisitInvocationExpression(InvokationExpressionSyntax* node) override;
        void VisitIndexatorExpression(IndexatorExpressionSyntax* node) override;
        void VisitMemberAccessExpression(MemberAccessExpressionSyntax* node) override;

        void VisitCastExpression(CastExpressionSyntax* node) override;
        void VisitIsExpression(IsExpressionSyntax* node) override;

private:
        void EmitMethodCall(MethodSymbol* method);
	};
}
