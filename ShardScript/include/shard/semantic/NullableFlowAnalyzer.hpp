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

		using State = gmt::unordered_map<SyntaxSymbol*, NullState>;

		struct ConditionalStates
		{
			State whenTrue;
			State whenFalse;
		};

		State m_state;
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

		SyntaxSymbol* tracked_root(const ExpressionSyntax* expression);
		StructSymbol* symbol_type(SyntaxSymbol* symbol) const;

		void collect_assigned(const SyntaxNode* node, gmt::unordered_set<SyntaxSymbol*>& assigned);
		void collect_assigned_expression(const ExpressionSyntax* expression, gmt::unordered_set<SyntaxSymbol*>& assigned);
		void widen_assigned(const gmt::unordered_set<SyntaxSymbol*>& assigned);
		void apply_assignment(const BinaryExpressionSyntax* node);

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
