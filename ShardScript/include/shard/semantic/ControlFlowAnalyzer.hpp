#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxToken.hpp>
#include <shard/semantic/SemanticVisitor.hpp>

#include <shard/semantic/symbols/FieldSymbol.hpp>
#include <shard/semantic/symbols/MethodSymbol.hpp>

#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace shard
{
	class PropertySymbol;
	class StructSymbol;

	class SHARD_API ControlFlowAnalyzer : public SemanticVisitor
	{
		using AssignmentSet = std::unordered_set<gmt::Ref<SyntaxSymbol>>;

		struct TryCatchFrame
		{
			std::unordered_set<gmt::Ref<StructSymbol>> caughtTypes;
			bool catchesAll = false;
		};

		struct CallEdge
		{
			gmt::Ref<MethodSymbol> callee;
			SyntaxToken callSite;
			std::unordered_set<gmt::Ref<StructSymbol>> caughtTypes;
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

		gmt::Ref<MethodSymbol> m_currentMethod;
		std::vector<gmt::Ref<MethodSymbol>> m_methodStack;

		std::vector<TryCatchFrame> m_tryCatchStack;
		std::vector<gmt::Ref<StructSymbol>> m_catchExceptionStack;
		std::unordered_set<gmt::Ref<StructSymbol>> m_effectiveCaughtTypes;
		int m_catchAllDepth = 0;

		std::unordered_map<gmt::Ref<MethodSymbol>, std::vector<CallEdge>> m_callGraph;
		std::unordered_set<gmt::Ref<const ExpressionSyntax>> m_tryWrappedExpressions;

		void enter_member_body(gmt::Ref<MethodSymbol> method);
		void leave_member_body(gmt::Ref<MethodSymbol> method);

		void scan_expression(gmt::Ref<const ExpressionSyntax> expression);

		static bool is_increment_decrement_operator(TokenType type);
		bool is_this_expression(gmt::Ref<const ExpressionSyntax> expression);

		void record_field_or_property_write(gmt::Ref<const MemberAccessExpressionSyntax> node);
		void record_indexator_write(gmt::Ref<const IndexatorExpressionSyntax> node);
		void record_callee_effects(gmt::Ref<MethodSymbol> callee, const SyntaxToken& callSite, bool isAwaited = false, bool isTryWrapped = false);

		bool is_try_wrapped(gmt::Ref<const ExpressionSyntax> expression) const;
		void add_thrown_type(std::unordered_set<gmt::Ref<StructSymbol>>& thrownTypes, gmt::Ref<StructSymbol> type);
		bool call_edge_catches(const CallEdge& edge, gmt::Ref<StructSymbol> thrownType);

		void propagate_effects();
		void report_effect_diagnostics();
		void report_edge_diagnostics(const CallEdge& edge);

		void push_try_catch(gmt::Span<const gmt::Ref<CatchClauseSyntax>> clauses);
		void pop_try_catch();
		bool is_caught_by_active_try(gmt::Ref<StructSymbol> thrownType);
		void rebuild_effective_caught_types();

		void push_catch_exception(gmt::Ref<StructSymbol> type);
		void pop_catch_exception();
		gmt::Ref<StructSymbol> current_catch_exception_type() const;

		gmt::Ref<StructSymbol> lookup_catch_type(gmt::Ref<const CatchClauseSyntax> clause);

		static FlowState merge_branches(FlowState a, FlowState b);
		static FlowState without_breaks_and_continues(FlowState state);
		static FlowState without_throws(FlowState state);

	public:
		static bool is_assignment_operator(TokenType type);

	private:
		FlowState analyze_statement(gmt::Ref<const StatementSyntax> node);
		FlowState analyze_statements_block(gmt::Ref<const StatementsBlockSyntax> node);
		FlowState analyze_body(gmt::Ref<const BlockSyntax> body);
		FlowState analyze_conditional_chain(gmt::Ref<const ConditionalClauseBaseSyntax> node);
		FlowState analyze_try_statement(gmt::Ref<const TryStatementSyntax> node);
		FlowState analyze_throw_statement(gmt::Ref<const ThrowStatementSyntax> statement);
		FlowState analyze_loop_body(gmt::Ref<const StatementsBlockSyntax> body);

		void analyze_member_body(gmt::Ref<MethodSymbol> method, gmt::Ref<const BlockSyntax> body, const TextLocation& errorLocation);
		void report_missing_return(const TextLocation& location, gmt::Ref<StructSymbol> returnType);

		void declare_parameters(gmt::Ref<MethodSymbol> method);

		void mark_assigned(gmt::Ref<SyntaxSymbol> variable);
		bool is_assigned(gmt::Ref<SyntaxSymbol> variable) const;
		void analyze_expression_for_assignment(gmt::Ref<const ExpressionSyntax> expression);
		void analyze_expression_for_use(gmt::Ref<const ExpressionSyntax> expression);
		void analyze_variable_statement(gmt::Ref<const VariableStatementSyntax> node);

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
