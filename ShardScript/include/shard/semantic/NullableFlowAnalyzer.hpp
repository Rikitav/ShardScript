#pragma once
#include <shard/Definitions.hpp>

#include <shard/semantic/SemanticVisitor.hpp>

#include <gmt/Containers.hpp>

namespace shard
{
	class SHARD_API NullableFlowAnalyzer final : public SemanticVisitor
	{
		enum class NullState
		{
			DEFINITELY_NULL,
			MAYBE_NULL,
			NOT_NULL
		};

		// flow facts are keyed by a path id: a root local or parameter plus the
		// member chain read from it, so `h.Item` carries a fact of its own
		struct PathNode
		{
			SyntaxSymbol* root = nullptr;   // set on root nodes only
			int parent = -1;                // path this node extends, -1 for a root
			SyntaxSymbol* member = nullptr; // field or property reached from the parent
		};

		using State = gmt::unordered_map<int, NullState>;

		struct ConditionalStates
		{
			State whenTrue;
			State whenFalse;
		};

		State m_state;
		gmt::vector<PathNode> m_paths;
		gmt::unordered_map<SyntaxSymbol*, int> m_rootPaths;
		gmt::unordered_map<int, gmt::unordered_map<SyntaxSymbol*, int>> m_memberPaths;
		gmt::vector<gmt::vector<const StatementSyntax*>> m_deferScopes;
		gmt::unordered_set<const ExpressionSyntax*> m_reported;

		static NullState join(NullState left, NullState right);
		static State merge(const State& left, const State& right);

		NullState state_of_expression(const ExpressionSyntax* expression);
		void analyze_expression(const ExpressionSyntax* expression);
		ConditionalStates analyze_condition(const ExpressionSyntax* expression);

		bool analyze_statement(const StatementSyntax* statement);
		bool analyze_block(const BlockSyntax* block);
		bool analyze_clause(const ConditionalClauseBaseSyntax* clause);
		bool analyze_conditional_clause(const ConditionalClauseSyntax* clause, bool negate);

		int root_path(SyntaxSymbol* symbol);
		int member_path(int parent, SyntaxSymbol* member);
		int tracked_path(const ExpressionSyntax* expression);
		bool path_within(int id, int ancestor) const;
		bool is_member_path(int id) const;
		void invalidate_member_paths();
		StructSymbol* symbol_type(SyntaxSymbol* symbol) const;

		void collect_assigned(const SyntaxNode* node, gmt::unordered_set<int>& assigned);
		void collect_assigned_expression(const ExpressionSyntax* expression, gmt::unordered_set<int>& assigned);
		void widen_assigned(const gmt::unordered_set<int>& assigned);
		void apply_assignment(const BinaryExpressionSyntax* node);
		void analyze_deferred_body(const StatementSyntax* statement);

		void report_unwrap(const ExpressionSyntax* expression);
		void declare_parameters(MethodSymbol* method);
		void analyze_member_body(MethodSymbol* method, const BlockSyntax* body);
		void analyze_lambda(const LambdaExpressionSyntax* node);

	public:
		NullableFlowAnalyzer(SemanticModel& model, DiagnosticsContext& diagnostics);

		void analyze(SyntaxTree& tree);

	protected:
		void visit_function_declaration(const FunctionDeclarationSyntax* node) override;
		void visit_operator_declaration(const OperatorDeclarationSyntax* node) override;
		void visit_constructor_declaration(const ConstructorDeclarationSyntax* node) override;
		void visit_accessor_declaration(const AccessorDeclarationSyntax* node) override;
		void visit_lambda_expression(const LambdaExpressionSyntax* node) override;
	};
}
