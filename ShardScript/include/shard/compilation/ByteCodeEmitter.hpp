#pragma once
#include <shard/Definitions.hpp>

#include <shard/semantic/SemanticVisitor.hpp>

#include <shard/compilation/ByteCodeEncoder.hpp>
#include <shard/compilation/OperationCode.hpp>
#include <shard/compilation/VirtualProgramImage.hpp>

#include <shard/pragmatic/PlacementVerdict.hpp>

#include <gmt/Ref.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace shard
{
	class SemanticModel;
	class PragmaticModel;
	class FrameLayout;
	class TypeShape;
	class SyntaxTree;
	class MethodSymbol;
	class StructSymbol;
	class FieldSymbol;
	class VariableSymbol;
	class ParameterSymbol;
	class ExpressionSyntax;
	class StatementSyntax;
	class MemberAccessExpressionSyntax;
	class IdentifierNameTypeSyntax;
	class IndexatorExpressionSyntax;
	class BinaryExpressionSyntax;
	class ArgumentsListSyntax;
	class BlockSyntax;
	class LambdaExpressionSyntax;
	class ObjectExpressionSyntax;

	class SHARD_API ByteCodeEmitter final : public SemanticVisitor
	{
		struct loop_scope
		{
			std::size_t loopStart;
			std::size_t deferScopeDepth;
			std::vector<std::size_t> blockEndBacktracks;
			std::vector<std::size_t> loopEndBacktracks;
		};

		VirtualProgramImage& m_image;
		PragmaticModel& m_pragmatic;
		ByteCodeEncoder m_encoder;

		gmt::Ref<MethodSymbol> m_generatingFor;
		FrameLayout* m_layout = nullptr;
		std::size_t m_methodStart = 0;

		std::vector<loop_scope> m_loops;
		std::vector<std::vector<std::size_t>> m_clauseEndBacktracks;
		std::vector<std::size_t> m_deferScopes;

		std::int32_t m_evalDepth = 0;
		std::uint32_t m_maxEvalDepth = 0;
		std::size_t m_maxEvalPayload = 0;
		bool m_evalPoisoned = false;

		std::vector<gmt::Ref<MethodSymbol>> m_entryPointCandidates;

		std::vector<std::pair<gmt::Ref<const LambdaExpressionSyntax>, gmt::Ref<MethodSymbol>>> m_pendingLambdaBodies;

		const TypeShape* shape_of(gmt::Ref<StructSymbol> type);
		std::size_t payload_of(gmt::Ref<StructSymbol> type);

		void eval_push(std::size_t payload);
		void eval_push_type(gmt::Ref<StructSymbol> type);
		void eval_pop(std::size_t count = 1);

		std::size_t mark_position() const;
		void patch_jump(std::size_t at);
		void patch_target(std::size_t at, std::size_t target);

		std::optional<std::uint16_t> slot_of(gmt::Ref<SyntaxSymbol> symbol) const;
		std::optional<std::uint16_t> scalar_field_slot(gmt::Ref<SyntaxSymbol> variable, gmt::Ref<FieldSymbol> field) const;
		std::uint16_t allocate_temporary(gmt::Ref<StructSymbol> type, PlacementVerdict verdict);
		std::uint32_t field_slot_index(gmt::Ref<FieldSymbol> field) const;
		PlacementVerdict local_verdict(gmt::Ref<SyntaxSymbol> symbol) const;
		PlacementVerdict site_verdict(gmt::Ref<const ExpressionSyntax> site) const;

		void begin_method(gmt::Ref<MethodSymbol> method);
		void end_method();
		void emit_body(gmt::Ref<const BlockSyntax> body);
		bool method_supported(gmt::Ref<MethodSymbol> method, gmt::Ref<const SyntaxNode> blame);

		void emit_defer_drain(std::size_t count);
		void emit_scope_defers();
		void emit_loop_defers();
		void emit_all_defers();
		void register_defer();
		void emit_defer_region(gmt::Ref<const SyntaxNode> bodyNode);

		void emit_condition_jump(gmt::Ref<const ExpressionSyntax> condition, bool jumpWhenTrue);
		void emit_assignment(const BinaryExpressionSyntax* node);
		void emit_store_to(const ExpressionSyntax* target);
		void emit_compound_assignment(const BinaryExpressionSyntax* node, TokenType arithmetic);
		void emit_identifier_load(gmt::Ref<const SyntaxNode> node, gmt::Ref<const ExpressionSyntax> previous);
		static bool is_identifier_target(const ExpressionSyntax* expression);
		void emit_instance_field_load(gmt::Ref<FieldSymbol> field, gmt::Ref<const ExpressionSyntax> previous);
		gmt::Ref<FieldSymbol> capture_field_of(gmt::Ref<SyntaxSymbol> bound) const;
		void emit_method_call(gmt::Ref<MethodSymbol> method, gmt::Ref<const ExpressionSyntax> receiver, const ArgumentsListSyntax* arguments, bool receiverFirst);
		void emit_arguments_reversed(const ArgumentsListSyntax* arguments);
		void emit_object_construction(const ObjectExpressionSyntax* node, std::optional<std::uint16_t> directSlot = std::nullopt);
		bool emit_arithmetic_op(TokenType type);
		void emit_field_initializers(gmt::Ref<StructSymbol> ownerType);

		void pick_entry_points();
		bool is_entry_point_candidate(gmt::Ref<MethodSymbol> method);

		void unsupported(gmt::Ref<const SyntaxNode> blame, const wchar_t* feature);

	public:
		ByteCodeEmitter(VirtualProgramImage& image, SemanticModel& model, PragmaticModel& pragmatic, DiagnosticsContext& diagnostics);

		void emit(SyntaxTree& tree);

		void visit_function_declaration(const FunctionDeclarationSyntax* node) override;
		void visit_operator_declaration(const OperatorDeclarationSyntax* node) override;
		void visit_constructor_declaration(const ConstructorDeclarationSyntax* node) override;
		void visit_accessor_declaration(const AccessorDeclarationSyntax* node) override;
		void visit_lambda_expression(const LambdaExpressionSyntax* node) override;

		void visit_statements_block(const StatementsBlockSyntax* node) override;
		void visit_expression_statement(const ExpressionStatementSyntax* node) override;
		void visit_variable_statement(const VariableStatementSyntax* node) override;
		void visit_return_statement(const ReturnStatementSyntax* node) override;
		void visit_if_statement(const IfStatementSyntax* node) override;
		void visit_unless_statement(const UnlessStatementSyntax* node) override;
		void emit_conditional_chain(const ConditionalClauseBaseSyntax* head);
		void visit_while_statement(const WhileStatementSyntax* node) override;
		void visit_until_statement(const UntilStatementSyntax* node) override;
		void visit_for_in_statement(const ForInStatementSyntax* node) override;
		void visit_break_statement(const BreakStatementSyntax* node) override;
		void visit_continue_statement(const ContinueStatementSyntax* node) override;
		void visit_throw_statement(const ThrowStatementSyntax* node) override;
		void visit_try_statement(const TryStatementSyntax* node) override;
		void visit_defer_statement(const DeferStatementSyntax* node) override;
		void visit_defer_block(const DeferBlockSyntax* node) override;
		void visit_defer_variable(const DeferVariableSyntax* node) override;

		void visit_literal_expression(const LiteralExpressionSyntax* node) override;
		void visit_binary_expression(const BinaryExpressionSyntax* node) override;
		void visit_unary_expression(const UnaryExpressionSyntax* node) override;
		void visit_member_access_expression(const MemberAccessExpressionSyntax* node) override;
		void visit_identifier_name_type(const IdentifierNameTypeSyntax* node) override;
		void visit_invokation_expression(const InvokationExpressionSyntax* node) override;
		void visit_indexator_expression(const IndexatorExpressionSyntax* node) override;
		void visit_is_expression(const IsExpressionSyntax* node) override;
		void visit_cast_expression(const CastExpressionSyntax* node) override;
		void visit_if_expression(const IfExpressionSyntax* node) override;
		void visit_await_expression(const AwaitExpressionSyntax* node) override;
		void visit_range_expression(const RangeExpressionSyntax* node) override;
		void visit_collection_expression(const CollectionExpressionSyntax* node) override;
		void visit_object_creation_expression(const ObjectExpressionSyntax* node) override;
	};
}
