#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxToken.hpp>
#include <shard/semantic/SemanticVisitor.hpp>

#include <shard/semantic/symbols/FieldSymbol.hpp>
#include <shard/semantic/symbols/MethodSymbol.hpp>

#include <gmt/Containers.hpp>

#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace shard
{
	class PropertySymbol;
	class StructSymbol;

	class SHARD_API ControlFlowAnalyzer : public SemanticVisitor
	{
		using AssignmentSet = gmt::unordered_set<SyntaxSymbol*>;

		struct TryCatchFrame
		{
			gmt::unordered_set<StructSymbol*> caughtTypes;
			bool catchesAll = false;
		};

		struct CallEdge
		{
			MethodSymbol* callee = nullptr;
			SyntaxToken callSite;
			gmt::unordered_set<StructSymbol*> caughtTypes;
			bool catchesAll = false;
			bool isAwaited = false;
			bool isTryWrapped = false;
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

		static bool has_flag(FlowState state, FlowState flag)
		{
			return static_cast<unsigned int>(state & flag) != 0;
		}

		static bool is_terminal(FlowState state)
		{
			return !has_flag(state, FlowState::Normal);
		}

		AssignmentSet m_assignedVariables;
		int m_loopDepth = 0;
		bool m_currentMethodIsAsync = false;

		MethodSymbol* m_currentMethod = nullptr;
		gmt::vector<MethodSymbol*> m_methodStack;

		gmt::vector<TryCatchFrame> m_tryCatchStack;
		gmt::vector<StructSymbol*> m_catchExceptionStack;
		gmt::unordered_set<StructSymbol*> m_effectiveCaughtTypes;
		int m_catchAllDepth = 0;

		gmt::unordered_map<MethodSymbol*, gmt::vector<CallEdge>> m_callGraph;
		gmt::unordered_set<const ExpressionSyntax*> m_tryWrappedExpressions;

		void enter_member_body(MethodSymbol* method);
		void leave_member_body(MethodSymbol* method);

		void scan_expression(const ExpressionSyntax* expression);

		static bool is_increment_decrement_operator(TokenType type);
		bool is_this_expression(const ExpressionSyntax* expression);

		void record_field_or_property_write(const MemberAccessExpressionSyntax* node);
		void record_indexator_write(const IndexatorExpressionSyntax* node);
		void record_callee_effects(MethodSymbol* callee, const SyntaxToken& callSite, bool isAwaited = false, bool isTryWrapped = false);

		bool is_try_wrapped(const ExpressionSyntax* expression) const;
		void add_thrown_type(gmt::unordered_set<StructSymbol*>& thrownTypes, StructSymbol* type);
		bool call_edge_catches(const CallEdge& edge, StructSymbol* thrownType);

		void propagate_effects();
		void report_effect_diagnostics();
		void report_edge_diagnostics(const CallEdge& edge);

		void push_try_catch(std::span<CatchClauseSyntax* const> clauses);
		void pop_try_catch();
		bool is_caught_by_active_try(StructSymbol* thrownType);
		void rebuild_effective_caught_types();

		void push_catch_exception(StructSymbol* type);
		void pop_catch_exception();
		StructSymbol* current_catch_exception_type() const;

		StructSymbol* lookup_catch_type(const CatchClauseSyntax* clause);

		static FlowState merge_branches(FlowState a, FlowState b);
		static FlowState without_breaks_and_continues(FlowState state);
		static FlowState without_throws(FlowState state);

	public:
		static bool is_assignment_operator(TokenType type);

	private:
		FlowState analyze_statement(const StatementSyntax* node);
		FlowState analyze_statements_block(const StatementsBlockSyntax* node);
		FlowState analyze_body(const BlockSyntax* body);
		FlowState analyze_conditional_chain(const ConditionalClauseBaseSyntax* node);
		FlowState analyze_try_statement(const TryStatementSyntax* node);
		FlowState analyze_throw_statement(const ThrowStatementSyntax* statement);
		FlowState analyze_loop_body(const StatementsBlockSyntax* body);

		void analyze_member_body(MethodSymbol* method, const BlockSyntax* body, const TextLocation& errorLocation);
		void report_missing_return(const TextLocation& location, StructSymbol* returnType);

		void declare_parameters(MethodSymbol* method);

		void mark_assigned(SyntaxSymbol* variable);
		bool is_assigned(SyntaxSymbol* variable) const;
		void analyze_expression_for_assignment(const ExpressionSyntax* expression);
		void analyze_expression_for_use(const ExpressionSyntax* expression);
		void analyze_variable_statement(const VariableStatementSyntax* node);

	public:
		ControlFlowAnalyzer(SemanticModel& model, DiagnosticsContext& diagnostics);

		void analyze(SyntaxTree& tree);

	protected:
		void visit_function_declaration(const FunctionDeclarationSyntax* node) override;
		void visit_operator_declaration(const OperatorDeclarationSyntax* node) override;
		void visit_constructor_declaration(const ConstructorDeclarationSyntax* node) override;
		void visit_lambda_expression(const LambdaExpressionSyntax* node) override;

		void visit_binary_expression(const BinaryExpressionSyntax* node) override;
		void visit_unary_expression(const UnaryExpressionSyntax* node) override;
		void visit_invokation_expression(const InvokationExpressionSyntax* node) override;
		void visit_object_creation_expression(const ObjectExpressionSyntax* node) override;
		void visit_await_expression(const AwaitExpressionSyntax* node) override;
	};
}
