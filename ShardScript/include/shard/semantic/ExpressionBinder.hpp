#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxVisitor.hpp>
#include <shard/semantic/SemanticModel.hpp>
#include <shard/semantic/SemanticVisitor.hpp>
#include <shard/semantic/SymbolFactory.hpp>

#include <shard/parsing/nodes/TranslationUnitSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarationSyntax.hpp>
#include <shard/parsing/nodes/TypeSyntax.hpp>

#include <shard/parsing/nodes/directives/UsingDirectiveSyntax.hpp>

#include <shard/parsing/nodes/lists/ParametersListSyntax.hpp>
#include <shard/parsing/nodes/lists/ArgumentsListSyntax.hpp>
#include <shard/parsing/nodes/lists/TypeArgumentsListSyntax.hpp>

#include <shard/parsing/nodes/blocks/ArrowClauseSyntax.hpp>
#include <shard/parsing/nodes/blocks/StatementsBlockSyntax.hpp>

#include <shard/parsing/nodes/statements/ConditionalClauseSyntax.hpp>
#include <shard/parsing/nodes/statements/DeferBlockSyntax.hpp>
#include <shard/parsing/nodes/statements/DeferStatementSyntax.hpp>
#include <shard/parsing/nodes/statements/DeferVariableSyntax.hpp>
#include <shard/parsing/nodes/statements/ExpressionStatementSyntax.hpp>
#include <shard/parsing/nodes/statements/ReturnStatementSyntax.hpp>
#include <shard/parsing/nodes/statements/ThrowStatementSyntax.hpp>
#include <shard/parsing/nodes/statements/TryStatementSyntax.hpp>
#include <shard/parsing/nodes/statements/VariableStatementSyntax.hpp>

#include <shard/parsing/nodes/expressions/AwaitExpressionSyntax.hpp>
#include <shard/parsing/nodes/expressions/BinaryExpressionSyntax.hpp>
#include <shard/parsing/nodes/expressions/CastExpressionSyntax.hpp>
#include <shard/parsing/nodes/expressions/CollectionExpressionSyntax.hpp>
#include <shard/parsing/nodes/expressions/IfExpressionSyntax.hpp>
#include <shard/parsing/nodes/expressions/IndexatorExpressionSyntax.hpp>
#include <shard/parsing/nodes/expressions/InvokationExpressionSyntax.hpp>
#include <shard/parsing/nodes/expressions/IsExpressionSyntax.hpp>
#include <shard/parsing/nodes/expressions/LambdaExpressionSyntax.hpp>
#include <shard/parsing/nodes/expressions/LiteralExpressionsSyntax.hpp>
#include <shard/parsing/nodes/expressions/MemberAccessExpressionSyntax.hpp>
#include <shard/parsing/nodes/expressions/ObjectExpressionSyntax.hpp>
#include <shard/parsing/nodes/expressions/RangeExpressionSyntax.hpp>
#include <shard/parsing/nodes/expressions/UnaryExpressionSyntax.hpp>

#include <shard/parsing/nodes/loops/ForInStatementSyntax.hpp>
#include <shard/parsing/nodes/loops/UntilStatementSyntax.hpp>
#include <shard/parsing/nodes/loops/WhileStatementSyntax.hpp>

#include <shard/parsing/nodes/members/AccessorDeclarationSyntax.hpp>
#include <shard/parsing/nodes/members/ConstructorDeclarationSyntax.hpp>
#include <shard/parsing/nodes/members/EnumDeclarationSyntax.hpp>
#include <shard/parsing/nodes/members/FieldDeclarationSyntax.hpp>
#include <shard/parsing/nodes/members/FunctionDeclarationSyntax.hpp>
#include <shard/parsing/nodes/members/IndexatorDeclarationSyntax.hpp>
#include <shard/parsing/nodes/members/InterfaceDeclarationSyntax.hpp>
#include <shard/parsing/nodes/members/OperatorDeclarationSyntax.hpp>
#include <shard/parsing/nodes/members/PropertyDeclarationSyntax.hpp>
#include <shard/parsing/nodes/members/StructDeclarationSyntax.hpp>

#include <shard/parsing/nodes/types/ArrayTypeSyntax.hpp>
#include <shard/parsing/nodes/types/GenericTypeSyntax.hpp>
#include <shard/parsing/nodes/types/IdentifierNameTypeSyntax.hpp>
#include <shard/parsing/nodes/types/NullableTypeSyntax.hpp>
#include <shard/parsing/nodes/types/PredefinedTypeSyntax.hpp>
#include <shard/parsing/nodes/types/QualifiedNameTypeSyntax.hpp>

#include <span>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace shard
{
	class OperatorSymbol;

	class SHARD_API ExpressionBinder final : public SemanticVisitor
	{
		SymbolFactory m_factory;
		std::unordered_set<std::wstring_view> m_ambiguousNames;
		std::unordered_set<gmt::Ref<const ExpressionSyntax>> m_staticContextExpressions;

		void visit_expression(gmt::Ref<const ExpressionSyntax> expression);

		void set_expression_type(gmt::Ref<const ExpressionSyntax> expression, gmt::Ref<StructSymbol> type);
		gmt::Ref<StructSymbol> get_expression_type(gmt::Ref<const ExpressionSyntax> expression);
		gmt::Ref<StructSymbol> find_target_return_type();
		gmt::Ref<StructSymbol> resolve_left_denotation();
		bool get_is_static_context(gmt::Ref<const ExpressionSyntax> expression) const;

		gmt::Ref<StructSymbol> analyze_literal_expression(const LiteralExpressionSyntax* node);
		gmt::Ref<StructSymbol> analyze_number_literal(const LiteralExpressionSyntax* node);
		gmt::Ref<StructSymbol> analyze_double_literal(const LiteralExpressionSyntax* node);
		gmt::Ref<StructSymbol> analyze_binary_expression(const BinaryExpressionSyntax* node);
		gmt::Ref<StructSymbol> analyze_unary_expression(const UnaryExpressionSyntax* node);
		gmt::Ref<StructSymbol> analyze_object_expression(const ObjectExpressionSyntax* node);
		gmt::Ref<StructSymbol> analyze_collection_expression(const CollectionExpressionSyntax* node);
		gmt::Ref<StructSymbol> analyze_member_access_expression(const MemberAccessExpressionSyntax* node, gmt::Ref<StructSymbol> currentType);
		gmt::Ref<StructSymbol> analyze_identifier_expression(SyntaxToken identifier, const SyntaxNode* node, std::optional<gmt::Ref<const SyntaxNode>> nodeRef);
		gmt::Ref<StructSymbol> resolve_symbol_type(gmt::Ref<SyntaxSymbol> symbol, SyntaxToken identifier, gmt::Ref<const ExpressionSyntax> previous, gmt::Ref<StructSymbol> currentType, std::optional<gmt::Ref<const SyntaxNode>> nodeRef, TextLocation location);
		gmt::Ref<StructSymbol> analyze_property_access_expression(SyntaxToken identifier, bool assignmentContext, gmt::Ref<const SyntaxNode> callSite, gmt::Ref<const ExpressionSyntax> previous, gmt::Ref<PropertySymbol> property, gmt::Ref<StructSymbol> currentType);
		gmt::Ref<StructSymbol> analyze_field_keyword_expression(const SyntaxNode* node);
		gmt::Ref<StructSymbol> analyze_invokation_expression(const InvokationExpressionSyntax* node, gmt::Ref<StructSymbol> currentType);
		gmt::Ref<StructSymbol> analyze_qualified_invokation_expression(const InvokationExpressionSyntax* node, gmt::Ref<NamespaceSymbol> namespaceSymbol);
		gmt::Ref<StructSymbol> analyze_indexator_expression(const IndexatorExpressionSyntax* node, gmt::Ref<StructSymbol> currentType);

		gmt::Ref<ConstructorSymbol> resolve_constructor(const ObjectExpressionSyntax* node);
		gmt::Ref<MethodSymbol> resolve_method(const InvokationExpressionSyntax* node, gmt::Ref<StructSymbol> currentType, std::vector<gmt::Ref<StructSymbol>>& outMethodTypeArgs, gmt::Ref<GenericTypeSymbol>& outGenericType);
		gmt::Ref<NamespaceSymbol> resolve_namespace_qualifier(gmt::Ref<const ExpressionSyntax> expression);
		gmt::Ref<IndexatorSymbol> resolve_indexator(const IndexatorExpressionSyntax* node, gmt::Ref<StructSymbol> currentType);
		gmt::Ref<DelegateTypeSymbol> try_resolve_delegate_invocation(const InvokationExpressionSyntax* node, gmt::Ref<SyntaxSymbol> lookup);

		gmt::Ref<StructSymbol> resolve_type_expression(gmt::Ref<const TypeSyntax> type);
		gmt::Ref<StructSymbol> substitute_type_parameters(gmt::Ref<StructSymbol> type, gmt::Ref<GenericTypeSymbol> genericType);
		gmt::Ref<StructSymbol> substitute_type_parameters(gmt::Ref<StructSymbol> type, gmt::Ref<GenericTypeSymbol> genericType, gmt::Ref<MethodSymbol> method, std::span<const gmt::Ref<StructSymbol>> methodTypeArgs);
		gmt::Ref<StructSymbol> substitute_method_type_parameter(gmt::Ref<StructSymbol> type, gmt::Ref<MethodSymbol> method, std::span<const gmt::Ref<StructSymbol>> methodTypeArgs);

		bool try_infer_type_argument(gmt::Ref<StructSymbol> pattern, gmt::Ref<StructSymbol> concrete, gmt::Ref<MethodSymbol> method, gmt::Ref<GenericTypeSymbol> genericType, std::vector<gmt::Ref<StructSymbol>>& outMethodTypeArgs);
		bool infer_method_type_arguments(gmt::Ref<MethodSymbol> method, std::span<const gmt::Ref<StructSymbol>> argTypes, gmt::Ref<GenericTypeSymbol> genericType, std::vector<gmt::Ref<StructSymbol>>& outMethodTypeArgs);
		bool infer_class_type_arguments(gmt::Ref<StructSymbol> classDef, gmt::Ref<MethodSymbol> method, std::span<const gmt::Ref<StructSymbol>> argTypes, std::vector<gmt::Ref<StructSymbol>>& outClassTypeArgs);

		int conversion_score(gmt::Ref<const StructSymbol> parameter, gmt::Ref<const StructSymbol> argument);
		int score_method_match(gmt::Ref<MethodSymbol> method, std::span<const gmt::Ref<StructSymbol>> argTypes, gmt::Ref<GenericTypeSymbol> genericType, std::vector<gmt::Ref<StructSymbol>>& outMethodTypeArgs);
		gmt::Ref<OperatorSymbol> resolve_operator_method(gmt::Ref<StructSymbol> ownerType, TokenType operatorToken, std::span<const gmt::Ref<StructSymbol>> parameterTypes);
		
		static bool is_assignment_operator(TokenType type);
		static bool is_assignment_context(const IndexatorExpressionSyntax* expression);
		
		bool match_method_arguments(SyntaxToken blameToken, std::span<const gmt::Ref<ParameterSymbol>> parameters, gmt::Span<const gmt::Ref<ArgumentSyntax>> arguments, gmt::Ref<GenericTypeSymbol> genericType, std::size_t parameterOffset = 0);
		bool collect_argument_types(const InvokationExpressionSyntax* node, std::vector<gmt::Ref<StructSymbol>>& outArgTypes);

		void import_namespace(gmt::Ref<const UsingDirectiveSyntax> node, gmt::Ref<NamespaceSymbol> namespaceSymbol);
		void visit_type_declaration(gmt::Ref<const TypeDeclarationSyntax> node);
		bool is_ambiguous_name(std::wstring_view name) const;
		gmt::Ref<NamespaceSymbol> find_namespace_by_name(gmt::Span<const SyntaxToken> name);

		void report_no_matching_method(std::wstring_view methodName, SyntaxToken blameToken);
		void report_no_matching_overload(std::wstring_view methodName, std::span<const gmt::Ref<StructSymbol>> argTypes, SyntaxToken blameToken);

	public:
		ExpressionBinder(SemanticModel& model, DiagnosticsContext& diagnostics);

		void visit_translation_unit(const TranslationUnitSyntax* node) override;
		void visit_using_directive(const UsingDirectiveSyntax* node) override;

		void visit_struct_declaration(const StructDeclarationSyntax* node) override;
		void visit_interface_declaration(const InterfaceDeclarationSyntax* node) override;
		void visit_enum_declaration(const EnumDeclarationSyntax* node) override;
		void visit_constructor_declaration(const ConstructorDeclarationSyntax* node) override;
		void visit_function_declaration(const FunctionDeclarationSyntax* node) override;
		void visit_operator_declaration(const OperatorDeclarationSyntax* node) override;
		void visit_field_declaration(const FieldDeclarationSyntax* node) override;
		void visit_property_declaration(const PropertyDeclarationSyntax* node) override;
		void visit_indexator_declaration(const IndexatorDeclarationSyntax* node) override;
		void visit_accessor_declaration(const AccessorDeclarationSyntax* node) override;

		void visit_variable_statement(const VariableStatementSyntax* node) override;
		void visit_return_statement(const ReturnStatementSyntax* node) override;
		void visit_expression_statement(const ExpressionStatementSyntax* node) override;
		void visit_if_statement(const IfStatementSyntax* node) override;
		void visit_unless_statement(const UnlessStatementSyntax* node) override;
		void visit_else_statement(const ElseStatementSyntax* node) override;
		void visit_while_statement(const WhileStatementSyntax* node) override;
		void visit_until_statement(const UntilStatementSyntax* node) override;
		void visit_for_in_statement(const ForInStatementSyntax* node) override;
		void visit_try_statement(const TryStatementSyntax* node) override;
		void visit_catch_clause(const CatchClauseSyntax* node) override;
		void visit_defer_statement(const DeferStatementSyntax* node) override;
		void visit_defer_block(const DeferBlockSyntax* node) override;
		void visit_defer_variable(const DeferVariableSyntax* node) override;
		void visit_statements_block(const StatementsBlockSyntax* node) override;
		void visit_arrow_clause(const ArrowClauseSyntax* node) override;

		void visit_literal_expression(const LiteralExpressionSyntax* node) override;
		void visit_identifier_name_type(const IdentifierNameTypeSyntax* node) override;
		void visit_qualified_name_type(const QualifiedNameTypeSyntax* node) override;
		void visit_generic_type(const GenericTypeSyntax* node) override;
		void visit_binary_expression(const BinaryExpressionSyntax* node) override;
		void visit_unary_expression(const UnaryExpressionSyntax* node) override;
		void visit_await_expression(const AwaitExpressionSyntax* node) override;
		void visit_object_creation_expression(const ObjectExpressionSyntax* node) override;
		void visit_collection_expression(const CollectionExpressionSyntax* node) override;
		void visit_range_expression(const RangeExpressionSyntax* node) override;
		void visit_lambda_expression(const LambdaExpressionSyntax* node) override;
		void visit_if_expression(const IfExpressionSyntax* node) override;
		void visit_member_access_expression(const MemberAccessExpressionSyntax* node) override;
		void visit_invokation_expression(const InvokationExpressionSyntax* node) override;
		void visit_indexator_expression(const IndexatorExpressionSyntax* node) override;
		void visit_cast_expression(const CastExpressionSyntax* node) override;
		void visit_is_expression(const IsExpressionSyntax* node) override;
	};
}
