#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxToken.hpp>
#include <shard/semantic/SemanticVisitor.hpp>

#include <shard/pragmatic/EscapeChannels.hpp>
#include <shard/pragmatic/PlacementVerdict.hpp>
#include <shard/pragmatic/PragmaticModel.hpp>

#include <gmt/Ref.hpp>

#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace shard
{
	class SHARD_API EscapeAnalyzer final : public SemanticVisitor
	{
		struct ExpressionRefs
		{
			std::vector<gmt::Ref<SyntaxSymbol>> locals;
			std::vector<gmt::Ref<const ExpressionSyntax>> sites;
		};

		class reference_collector;

		struct Frame
		{
			gmt::Ref<MethodSymbol> method;
			gmt::Ref<const ExpressionSyntax> closureSite; // null for member bodies
			std::unordered_set<gmt::Ref<SyntaxSymbol>> declared;
			bool seenAwait = false;
			std::vector<int> loopStack;
			std::unordered_set<int> loopsWithAwait;
		};

		// constraint graph over the locals and allocation sites of the whole
		// walk; member bodies never share edges, so a single graph suffices
		struct Graph
		{
			int nextId = 0;
			std::unordered_map<gmt::Ref<SyntaxSymbol>, int> localIds;
			std::unordered_map<gmt::Ref<const ExpressionSyntax>, int> siteIds;
			std::vector<EscapeLevel> levels;
			std::vector<EscapeChannel> channelSets;
			std::vector<gmt::Ref<MethodSymbol>> idOwners;
			std::vector<std::pair<int, int>> edges; // level[b] >= level[a]
			std::vector<std::tuple<int, EscapeLevel, EscapeChannel>> events;
		};

		PragmaticModel& m_pragmatic;

		Graph m_graph;
		std::vector<Frame> m_frameStack;
		int m_nextLoopId = 0;
		int m_inDefer = 0;

		int ensure_local_id(gmt::Ref<SyntaxSymbol> symbol);
		int ensure_site_id(gmt::Ref<const ExpressionSyntax> site);
		void add_edge(int from, int to);
		void add_event(int id, EscapeLevel level, EscapeChannel channel);
		bool join_node(int id, EscapeLevel level, EscapeChannel channel);

		void enter_frame(gmt::Ref<MethodSymbol> method, gmt::Ref<const ExpressionSyntax> closureSite);
		void leave_frame();
		bool in_frame() const;
		Frame& current_frame();

		void declare_local(gmt::Ref<SyntaxSymbol> symbol);
		int find_declaring_frame(gmt::Ref<SyntaxSymbol> symbol) const;

		void record_use(gmt::Ref<SyntaxSymbol> symbol);
		void record_capture(gmt::Ref<SyntaxSymbol> symbol);
		gmt::Ref<SyntaxSymbol> find_declared_local(std::wstring_view name) const;

		void collect_expression_refs(gmt::Ref<const ExpressionSyntax> expression, ExpressionRefs& refs, bool includeReceivers);
		void expose_expression(gmt::Ref<const ExpressionSyntax> expression, EscapeLevel level, EscapeChannel channel, bool includeReceivers = false);
		void alias_into(gmt::Ref<SyntaxSymbol> target, gmt::Ref<const ExpressionSyntax> sourceExpression);

		void register_allocation_site(const ExpressionSyntax* site);

		void declare_parameters(gmt::Ref<MethodSymbol> method);
		void analyze_member_body(gmt::Ref<MethodSymbol> method, gmt::Ref<const BlockSyntax> body);

		void finalize_graph();

	public:
		EscapeAnalyzer(SemanticModel& model, PragmaticModel& pragmatic, DiagnosticsContext& diagnostics);

		void analyze(SyntaxTree& tree);

	protected:
		void visit_function_declaration(const FunctionDeclarationSyntax* node) override;
		void visit_operator_declaration(const OperatorDeclarationSyntax* node) override;
		void visit_constructor_declaration(const ConstructorDeclarationSyntax* node) override;
		void visit_lambda_expression(const LambdaExpressionSyntax* node) override;

		void visit_variable_statement(const VariableStatementSyntax* node) override;
		void visit_for_in_statement(const ForInStatementSyntax* node) override;
		void visit_catch_clause(const CatchClauseSyntax* node) override;

		void visit_return_statement(const ReturnStatementSyntax* node) override;
		void visit_throw_statement(const ThrowStatementSyntax* node) override;
		void visit_expression_statement(const ExpressionStatementSyntax* node) override;
		void visit_arrow_clause(const ArrowClauseSyntax* node) override;

		void visit_while_statement(const WhileStatementSyntax* node) override;
		void visit_until_statement(const UntilStatementSyntax* node) override;

		void visit_defer_statement(const DeferStatementSyntax* node) override;
		void visit_defer_block(const DeferBlockSyntax* node) override;
		void visit_defer_variable(const DeferVariableSyntax* node) override;

		void visit_binary_expression(const BinaryExpressionSyntax* node) override;
		void visit_invokation_expression(const InvokationExpressionSyntax* node) override;
		void visit_object_creation_expression(const ObjectExpressionSyntax* node) override;
		void visit_collection_expression(const CollectionExpressionSyntax* node) override;
		void visit_await_expression(const AwaitExpressionSyntax* node) override;
		void visit_identifier_name_type(const IdentifierNameTypeSyntax* node) override;
	};
}
