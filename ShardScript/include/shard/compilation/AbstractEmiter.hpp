#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/compilation/ProgramVirtualImage.hpp>
#include <shard/compilation/ByteCodeEncoder.hpp>

#include <shard/analysis/DiagnosticsContext.hpp>
#include <shard/semantic/SemanticModel.hpp>
#include <shard/parsing/SyntaxTree.hpp>

#include <shard/parsing/SyntaxVisitor.hpp>

#include <shard/parsing/nodes/ArgumentsListSyntax.hpp>

#include <shard/parsing/nodes/Expressions/BinaryExpressionSyntax.hpp>
#include <shard/parsing/nodes/Expressions/CollectionExpressionSyntax.hpp>
#include <shard/parsing/nodes/Expressions/RangeExpressionSyntax.hpp>
#include <shard/parsing/nodes/Expressions/LambdaExpressionSyntax.hpp>
#include <shard/parsing/nodes/Expressions/LinkedExpressionSyntax.hpp>
#include <shard/parsing/nodes/Expressions/LiteralExpressionSyntax.hpp>
#include <shard/parsing/nodes/Expressions/ObjectExpressionSyntax.hpp>
#include <shard/parsing/nodes/Expressions/TernaryExpressionSyntax.hpp>
#include <shard/parsing/nodes/Expressions/UnaryExpressionSyntax.hpp>
#include <shard/parsing/nodes/Expressions/CastExpressionSyntax.hpp>
#include <shard/parsing/nodes/Expressions/IsExpressionSyntax.hpp>

#include <shard/parsing/nodes/Loops/ForStatementSyntax.hpp>
#include <shard/parsing/nodes/Loops/ForEachStatementSyntax.hpp>
#include <shard/parsing/nodes/Loops/ForInStatementSyntax.hpp>
#include <shard/parsing/nodes/Loops/UntilStatementSyntax.hpp>
#include <shard/parsing/nodes/Loops/WhileStatementSyntax.hpp>

#include <shard/parsing/nodes/MemberDeclarations/AccessorDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/ConstructorDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/EnumDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/MethodDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/OperatorDeclarationSyntax.hpp>

#include <shard/parsing/nodes/Statements/BreakStatementSyntax.hpp>
#include <shard/parsing/nodes/Statements/ConditionalClauseSyntax.hpp>
#include <shard/parsing/nodes/Statements/ContinueStatementSyntax.hpp>
#include <shard/parsing/nodes/Statements/ExpressionStatementSyntax.hpp>
#include <shard/parsing/nodes/Statements/DeferStatementSyntax.hpp>
#include <shard/parsing/nodes/Statements/ReturnStatementSyntax.hpp>
#include <shard/parsing/nodes/Statements/ThrowStatementSyntax.hpp>
#include <shard/parsing/nodes/Statements/TryStatementSyntax.hpp>
#include <shard/parsing/nodes/Statements/VariableStatementSyntax.hpp>

#include <shard/semantic/symbols/MethodSymbol.hpp>

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
		void VisitEnumDeclaration(EnumDeclarationSyntax* node) override;

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
        void VisitForInStatement(ForInStatementSyntax* node) override;
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
