#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/parsing/SyntaxVisitor.hpp>
#include <shard/semantic/ScopeVisitor.hpp>
#include <shard/analysis/DiagnosticsContext.hpp>
#include <shard/semantic/SemanticModel.hpp>
#include <shard/semantic/SymbolFactory.hpp>

#include <shard/semantic/symbols/TypeSymbol.hpp>
#include <shard/semantic/symbols/MethodSymbol.hpp>
#include <shard/semantic/symbols/VariableSymbol.hpp>
#include <shard/semantic/symbols/FieldSymbol.hpp>
#include <shard/semantic/symbols/GenericTypeSymbol.hpp>

#include <shard/parsing/nodes/Expressions/LinkedExpressionSyntax.hpp>
#include <shard/parsing/nodes/Expressions/ObjectExpressionSyntax.hpp>
#include <shard/parsing/nodes/Expressions/BinaryExpressionSyntax.hpp>
#include <shard/parsing/nodes/Expressions/UnaryExpressionSyntax.hpp>
#include <shard/parsing/nodes/Statements/TryStatementSyntax.hpp>

#include <unordered_set>
#include <unordered_map>
#include <vector>

namespace shard
{
	class SHARD_API FlowAnalyzer : public SyntaxVisitor, public ScopeVisitor
	{
		using AssignmentSet = std::unordered_set<VariableSymbol*>;

		struct CallEdge
		{
			MethodSymbol* Callee = nullptr;
			SyntaxToken CallSite;
			std::unordered_set<TypeSymbol*> CaughtTypes;
			bool CatchesAll = false;
			bool IsAwaited = false;

			bool Catches(TypeSymbol* thrownType) const;
		};

		struct TryCatchFrame
		{
			std::unordered_set<TypeSymbol*> CaughtTypes;
			bool CatchesAll = false;
		};

		enum class FlowState : unsigned int
		{
			Returns = 1,
			Breaks = 2,
			Continues = 4,
			Throws = 8,
			Normal = 16,
		};

		friend inline FlowState operator|(FlowState a, FlowState b)
		{
			return static_cast<FlowState>(static_cast<unsigned int>(a) | static_cast<unsigned int>(b));
		}

		friend inline FlowState operator&(FlowState a, FlowState b)
		{
			return static_cast<FlowState>(static_cast<unsigned int>(a) & static_cast<unsigned int>(b));
		}

		friend inline FlowState operator~(FlowState a)
		{
			return static_cast<FlowState>(~static_cast<unsigned int>(a) & 31u);
		}

		static inline bool HasFlag(FlowState state, FlowState flag)
		{
			return static_cast<unsigned int>(state & flag) != 0;
		}

		static inline bool IsTerminal(FlowState state)
		{
			return !HasFlag(state, FlowState::Normal);
		}

		SymbolFactory Factory;
		AssignmentSet _assignedVariables;
		int _loopDepth = 0;
		int _switchDepth = 0;
		bool _currentMethodIsAsync = false;

		// Side-effect scanning state
		MethodSymbol* _currentMethod = nullptr;

		std::vector<TryCatchFrame> _tryCatchStack;
		std::vector<TypeSymbol*> _catchExceptionStack;
		std::unordered_set<TypeSymbol*> _effectiveCaughtTypes;
		int _catchAllDepth = 0;

		void PushTryCatch(const std::vector<std::unique_ptr<CatchClauseSyntax>>& clauses);
		void PopTryCatch();
		bool IsCaughtByActiveTry(TypeSymbol* thrownType) const;
		void RebuildEffectiveCaughtTypes();

		void PushCatchException(TypeSymbol* type);
		void PopCatchException();
		TypeSymbol* CurrentCatchExceptionType() const;

		std::unordered_map<MethodSymbol*, std::vector<CallEdge>> _callGraph;

		void EnterMemberBody(MethodSymbol* method);
		void LeaveMemberBody(MethodSymbol* method);

		void ScanExpression(ExpressionSyntax* expression);
		void ScanStatement(StatementSyntax* statement);

		static bool IsAssignmentOperator(TokenType type);
		static bool IsIncrementDecrementOperator(TokenType type);
		static bool IsThisExpression(ExpressionSyntax* expression);

		void RecordFieldOrPropertyWrite(MemberAccessExpressionSyntax* node);
		void RecordIndexatorWrite(IndexatorExpressionSyntax* node);
		void RecordCalleeEffects(MethodSymbol* callee, const SyntaxToken& callSite, bool isAwaited = false);

		static bool IsTaskOrTaskLike(TypeSymbol* type);

		void PropagateEffects();
		void ReportEffectDiagnostics();

	public:
		FlowAnalyzer(SemanticModel& model, DiagnosticsContext& diagnostics);

		void Analyze(SyntaxTree& syntaxTree);

	protected:
		void VisitMethodDeclaration(MethodDeclarationSyntax* node) override;
		void VisitOperatorDeclaration(OperatorDeclarationSyntax* node) override;
		void VisitAccessorDeclaration(AccessorDeclarationSyntax* node) override;
		void VisitConstructorDeclaration(ConstructorDeclarationSyntax* node) override;
		void VisitLambdaExpression(LambdaExpressionSyntax* node) override;

		void VisitStatementsBlock(StatementsBlockSyntax* node) override;
		void VisitStatement(StatementSyntax* node) override;

		void VisitVariableStatement(VariableStatementSyntax* node);

		void VisitIfStatement(IfStatementSyntax* node) override;
		void VisitUnlessStatement(UnlessStatementSyntax* node) override;
		void VisitElseStatement(ElseStatementSyntax* node) override;
		void VisitConditionalClause(ConditionalClauseBaseSyntax* node) override;
		void VisitSwitchStatement(SwitchStatementSyntax* node) override;
		void VisitTryStatement(TryStatementSyntax* node) override;

		void VisitWhileStatement(WhileStatementSyntax* node) override;
		void VisitUntilStatement(UntilStatementSyntax* node) override;
		void VisitForStatement(ForStatementSyntax* node) override;
		void VisitForEachStatement(ForEachStatementSyntax* node) override;
		void VisitForInStatement(ForInStatementSyntax* node) override;

		void VisitBinaryExpression(BinaryExpressionSyntax* node) override;
		void VisitUnaryExpression(UnaryExpressionSyntax* node) override;
		void VisitInvocationExpression(InvokationExpressionSyntax* node) override;
		void VisitObjectCreationExpression(ObjectExpressionSyntax* node) override;
		void VisitIndexatorExpression(IndexatorExpressionSyntax* node) override;
		void VisitAwaitExpression(AwaitExpressionSyntax* node) override;

	private:
		static FlowState MergeBranches(FlowState a, FlowState b);

		FlowState AnalyzeStatement(StatementSyntax* node);
		FlowState AnalyzeStatementsBlock(StatementsBlockSyntax* node);
		FlowState AnalyzeConditionalChain(ConditionalClauseBaseSyntax* node);
		FlowState AnalyzeSwitchStatement(SwitchStatementSyntax* node);
		FlowState AnalyzeTryStatement(TryStatementSyntax* node);
		FlowState AnalyzeLoopBody(StatementsBlockSyntax* body);

		void AnalyzeMemberBody(SyntaxSymbol* symbol, StatementsBlockSyntax* body, SyntaxToken errorToken);
		void ReportMissingReturn(SyntaxToken token, TypeSymbol* returnType);

		static FlowState WithoutBreaksAndContinues(FlowState state);
		static FlowState WithoutThrows(FlowState state);

		void MarkAssigned(VariableSymbol* variable);
		bool IsAssigned(VariableSymbol* variable) const;
		void AnalyzeExpressionForAssignment(ExpressionSyntax* expression);
		void AnalyzeExpressionForUse(ExpressionSyntax* expression);
	};
}
