#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxToken.hpp>
#include <shard/semantic/SemanticVisitor.hpp>

#include <shard/pragmatic/EscapeChannels.hpp>
#include <shard/pragmatic/EscapeSummary.hpp>
#include <shard/pragmatic/PlacementVerdict.hpp>
#include <shard/pragmatic/PragmaticModel.hpp>

#include <gmt/Containers.hpp>
#include <gmt/InlineVector.hpp>
#include <gmt/Ref.hpp>

#include <cstdint>
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
			gmt::inline_vector<SyntaxSymbol*, 4> locals;
			gmt::inline_vector<const ExpressionSyntax*, 4> sites;
		};

		// copyable snapshot of ExpressionRefs for the memoization cache
		// (inline_vector is immovable, so the map cannot store it directly)
		struct CachedExpressionRefs
		{
			gmt::vector<SyntaxSymbol*> locals;
			gmt::vector<const ExpressionSyntax*> sites;
		};

		class reference_collector;

		struct Frame
		{
			MethodSymbol* method = nullptr;
			const ExpressionSyntax* closureSite = nullptr; // null for member bodies
			gmt::unordered_set<SyntaxSymbol*> declared;
			gmt::vector<SyntaxSymbol*> orderedLocals; // declaration order, persisted for the frame layout pass
			gmt::vector<const ExpressionSyntax*> returnedExpressions;
			bool seenAwait = false;
			gmt::vector<int> loopStack;
			gmt::unordered_set<int> loopsWithAwait;
		};

		// constraint graph over the locals and allocation sites of the whole walk;
		// member bodies never share edges, so a single graph suffices
		struct Graph
		{
			struct Event
			{
				int id;
				EscapeLevel level;
				EscapeChannel channel;
				bool cold; // the flow originates on an exceptional path (spec 4.6)
			};

			struct CallExposure
			{
				int nodeId;
				MethodSymbol* callee = nullptr;
				std::uint16_t parameterIndex; // 0-based into the callee summary (0 = receiver of an instance method)
				bool receiver;
				bool cold;
			};

			int nextId = 0;
			gmt::unordered_map<SyntaxSymbol*, int> localIds;
			gmt::unordered_map<const ExpressionSyntax*, int> siteIds;
			gmt::vector<EscapeLevel> levels;
			gmt::vector<EscapeChannel> channelSets;
			gmt::vector<EscapeChannel> coldChannels; // cold upward flows only; parallel to levels
			gmt::vector<bool> identityExposed;
			gmt::vector<MethodSymbol*> idOwners;
			gmt::vector<std::pair<int, int>> edges; // level[b] >= level[a]
			gmt::vector<Event> events;

			gmt::vector<CallExposure> callExposures;
		};

		struct WalkedMethod
		{
			MethodSymbol* method = nullptr;
			gmt::vector<const ExpressionSyntax*> returnedExpressions;
		};

		PragmaticModel& m_pragmatic;

		Graph m_graph;
		gmt::vector<Frame> m_frameStack;
		gmt::vector<WalkedMethod> m_walked;
		gmt::unordered_map<StructSymbol*, bool> m_scalarCache;
		gmt::unordered_map<const ExpressionSyntax*, CachedExpressionRefs> m_expressionRefs[2];
		gmt::vector<std::pair<int, const ExpressionSyntax*>> m_coldPoints;
		int m_nextLoopId = 0;
		int m_inDefer = 0;
		int m_coldDepth = 0;

		int ensure_local_id(SyntaxSymbol* symbol);
		int ensure_site_id(const ExpressionSyntax* site);
		void add_edge(int from, int to);
		void add_event(int id, EscapeLevel level, EscapeChannel channel, bool cold = false);
		bool join_node(int id, EscapeLevel level, EscapeChannel channel);
		bool join_cold(int id, EscapeChannel channel);
		void clear_graph();

		void enter_frame(MethodSymbol* method, const ExpressionSyntax* closureSite);
		void leave_frame();
		bool in_frame() const;
		Frame& current_frame();

		void declare_local(SyntaxSymbol* symbol);
		int find_declaring_frame(SyntaxSymbol* symbol) const;

		void record_use(SyntaxSymbol* symbol);
		void record_capture(SyntaxSymbol* symbol);
		void materialize_capture(const ExpressionSyntax* closureSite, SyntaxSymbol* symbol);
		SyntaxSymbol* find_declared_local(std::wstring_view name) const;

		void collect_expression_refs(const ExpressionSyntax* expression, ExpressionRefs& refs, bool includeReceivers);
		void expose_expression(const ExpressionSyntax* expression, EscapeLevel level, EscapeChannel channel, bool includeReceivers = false);
		bool is_fresh_invokation(const ExpressionSyntax* expression);
		void mark_identity(const ExpressionSyntax* expression, bool includeReceivers);
		void record_call_exposure(const ExpressionSyntax* expression, MethodSymbol* callee, std::uint16_t parameterIndex, bool receiver);
		void alias_into(SyntaxSymbol* target, const ExpressionSyntax* sourceExpression);

		void register_allocation_site(const ExpressionSyntax* site);

		void declare_parameters(MethodSymbol* method);
		void analyze_member_body(MethodSymbol* method, const BlockSyntax* body);

		void run_graph_fixpoint();
		gmt::unordered_map<MethodSymbol*, EscapeSummary> derive_summaries();
		void apply_call_exposures(const gmt::unordered_map<MethodSymbol*, EscapeSummary>& summaries);
		bool commit_summaries(const gmt::unordered_map<MethodSymbol*, EscapeSummary>& summaries);
		void assign_verdicts(const gmt::unordered_map<MethodSymbol*, EscapeSummary>& summaries);

		ParameterEscapeSummary derive_parameter_summary(MethodSymbol* method, ParameterSymbol* parameter, bool isReceiver);
		ReturnDisposition classify_returns(const WalkedMethod& walked, const gmt::unordered_map<MethodSymbol*, EscapeSummary>& summaries);
		int classify_return_expression(MethodSymbol* method, const ExpressionSyntax* expression, const gmt::unordered_map<MethodSymbol*, EscapeSummary>& summaries);
		int classify_local_roots(MethodSymbol* method, SyntaxSymbol* local);

		bool is_caller_placeable(MethodSymbol* owner) const;
		bool cone_closed(int id, const gmt::unordered_map<MethodSymbol*, EscapeSummary>& summaries) const;
		bool is_scalar_eligible(StructSymbol* type);
		bool participates_in_reference_cycle(StructSymbol* type);

		AllocationSiteVerdict classify_node(int id, StructSymbol* nodeType, const gmt::unordered_map<MethodSymbol*, EscapeSummary>& summaries);

		void record_cold_point(int id, const ExpressionSyntax* expression);
		bool reaches_site(int from, int to) const;
		void commit_promotion_points();

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
		void visit_is_expression(const IsExpressionSyntax* node) override;
		void visit_cast_expression(const CastExpressionSyntax* node) override;
		void visit_await_expression(const AwaitExpressionSyntax* node) override;
		void visit_identifier_name_type(const IdentifierNameTypeSyntax* node) override;
	};
}
