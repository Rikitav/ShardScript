#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxVisitor.hpp>
#include <shard/semantic/SemanticModel.hpp>
#include <shard/semantic/SemanticVisitor.hpp>
#include <shard/semantic/SymbolFactory.hpp>

#include <shard/parsing/nodes/TranslationUnitSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarationSyntax.hpp>
#include <shard/parsing/nodes/TypeSyntax.hpp>
#include <shard/parsing/nodes/BlockSyntax.hpp>

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

#include <gmt/Containers.hpp>

#include <memory_resource>
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
		gmt::unordered_set<std::wstring_view> m_ambiguousNames;
		gmt::unordered_set<const ExpressionSyntax*> m_staticContextExpressions;

		// lambdas whose parameter types could not be resolved yet; they bind
		// once an expected delegate type appears (overload resolution picks a
		// method, or a denotation channel supplies a target type)
		gmt::unordered_set<const LambdaExpressionSyntax*> m_pendingLambdas;

		// collects value-returning expressions of a bound body so a return
		// type can be inferred for declarations that omit one
		class return_expression_collector;

		void visit_expression(const ExpressionSyntax* expression);

		StructSymbol* infer_return_type_from_body(const BlockSyntax* body);

		void set_expression_type(const ExpressionSyntax* expression, StructSymbol* type);
		StructSymbol* get_expression_type(const ExpressionSyntax* expression);
		StructSymbol* find_target_return_type();
		SyntaxSymbol* find_target_return_owner();
		void apply_inferred_return_type(SyntaxSymbol* owner, StructSymbol* type);
		StructSymbol* resolve_left_denotation();
		bool get_is_static_context(const ExpressionSyntax* expression) const;

		StructSymbol* analyze_literal_expression(const LiteralExpressionSyntax* node);
		StructSymbol* analyze_number_literal(const LiteralExpressionSyntax* node);
		StructSymbol* analyze_double_literal(const LiteralExpressionSyntax* node);
		StructSymbol* analyze_binary_expression(const BinaryExpressionSyntax* node);
		StructSymbol* analyze_unary_expression(const UnaryExpressionSyntax* node);
		StructSymbol* analyze_object_expression(const ObjectExpressionSyntax* node);
		StructSymbol* analyze_collection_expression(const CollectionExpressionSyntax* node);
		StructSymbol* analyze_member_access_expression(const MemberAccessExpressionSyntax* node, StructSymbol* currentType);
		StructSymbol* analyze_identifier_expression(SyntaxToken identifier, const SyntaxNode* node, std::optional<const SyntaxNode*> nodeRef);
		StructSymbol* resolve_symbol_type(SyntaxSymbol* symbol, SyntaxToken identifier, const ExpressionSyntax* previous, StructSymbol* currentType, std::optional<const SyntaxNode*> nodeRef, TextLocation location);
		StructSymbol* analyze_property_access_expression(SyntaxToken identifier, bool assignmentContext, const SyntaxNode* callSite, const ExpressionSyntax* previous, PropertySymbol* property, StructSymbol* currentType);
		StructSymbol* analyze_field_keyword_expression(const SyntaxNode* node);
		StructSymbol* analyze_invokation_expression(const InvokationExpressionSyntax* node, StructSymbol* currentType);
		StructSymbol* analyze_qualified_invokation_expression(const InvokationExpressionSyntax* node, NamespaceSymbol* namespaceSymbol);
		StructSymbol* wrap_async_return_type(MethodSymbol* method, StructSymbol* type);
		StructSymbol* analyze_indexator_expression(const IndexatorExpressionSyntax* node, StructSymbol* currentType);

		ConstructorSymbol* resolve_constructor(const ObjectExpressionSyntax* node);
		MethodSymbol* resolve_method(const InvokationExpressionSyntax* node, StructSymbol* currentType, std::pmr::vector<StructSymbol*>& outMethodTypeArgs, GenericTypeSymbol*& outGenericType);
		NamespaceSymbol* resolve_namespace_qualifier(const ExpressionSyntax* expression);
		IndexatorSymbol* resolve_indexator(const IndexatorExpressionSyntax* node, StructSymbol* currentType);
		DelegateTypeSymbol* try_resolve_delegate_invocation(const InvokationExpressionSyntax* node, SyntaxSymbol* lookup, GenericTypeSymbol*& outInstantiation);

		StructSymbol* resolve_type_expression(const TypeSyntax* type);
		StructSymbol* substitute_type_parameters(StructSymbol* type, GenericTypeSymbol* genericType);
		StructSymbol* substitute_type_parameters(StructSymbol* type, GenericTypeSymbol* genericType, MethodSymbol* method, std::span<StructSymbol* const> methodTypeArgs);
		StructSymbol* substitute_method_type_parameter(StructSymbol* type, MethodSymbol* method, std::span<StructSymbol* const> methodTypeArgs);

		bool try_infer_type_argument(StructSymbol* pattern, StructSymbol* concrete, MethodSymbol* method, GenericTypeSymbol* genericType, std::pmr::vector<StructSymbol*>& outMethodTypeArgs);
		bool infer_method_type_arguments(MethodSymbol* method, std::span<StructSymbol* const> argTypes, GenericTypeSymbol* genericType, std::pmr::vector<StructSymbol*>& outMethodTypeArgs);
		bool infer_class_type_arguments(StructSymbol* classDef, MethodSymbol* method, std::span<StructSymbol* const> argTypes, gmt::vector<StructSymbol*>& outClassTypeArgs);

		int conversion_score(const StructSymbol* parameter, const StructSymbol* argument);
		int score_method_match(MethodSymbol* method, std::span<StructSymbol* const> argTypes, GenericTypeSymbol* genericType, std::pmr::vector<StructSymbol*>& outMethodTypeArgs, std::span<StructSymbol* const> explicitTypeArgs = {});
		bool check_method_type_constraints(MethodSymbol* method, std::span<StructSymbol* const> methodTypeArgs, SyntaxToken blameToken);
		bool collect_explicit_type_arguments(const InvokationExpressionSyntax* node, std::pmr::vector<StructSymbol*>& outTypeArgs);
		void bind_invocation_type_arguments(const InvokationExpressionSyntax* node, MethodSymbol* method, GenericTypeSymbol* genericType, std::span<StructSymbol* const> methodTypeArgs);
		OperatorSymbol* resolve_operator_method(StructSymbol* ownerType, TokenType operatorToken, std::span<StructSymbol* const> parameterTypes);
		
		static bool is_assignment_operator(TokenType type);
		static bool is_assignment_context(const IndexatorExpressionSyntax* expression);
		
		bool match_method_arguments(SyntaxToken blameToken, std::span<ParameterSymbol* const> parameters, std::span<ArgumentSyntax* const> arguments, GenericTypeSymbol* genericType, std::size_t parameterOffset = 0);
		bool collect_argument_types(const InvokationExpressionSyntax* node, std::pmr::vector<StructSymbol*>& outArgTypes);

		bool is_pending_lambda(const ExpressionSyntax* expression) const;
		bool try_bind_pending_lambda(const LambdaExpressionSyntax* node, StructSymbol* expectedType);
		void bind_pending_lambda_arguments(const InvokationExpressionSyntax* node, MethodSymbol* method, bool asExtension, GenericTypeSymbol* genericType, std::span<StructSymbol* const> argTypes, std::span<StructSymbol* const> explicitTypeArgs);
		void bind_lambda_body(const LambdaExpressionSyntax* node, const LambdaExpressionSyntax* nodeRef, DelegateTypeSymbol* delegateType, MethodSymbol* anonymousMethod);

		void import_namespace(const UsingDirectiveSyntax* node, NamespaceSymbol* namespaceSymbol);
		void visit_type_declaration(const TypeDeclarationSyntax* node);
		bool is_ambiguous_name(std::wstring_view name) const;
		NamespaceSymbol* find_namespace_by_name(std::span<const SyntaxToken> name);

		void report_no_matching_method(std::wstring_view methodName, SyntaxToken blameToken);
		void report_no_matching_overload(std::wstring_view methodName, std::span<StructSymbol* const> argTypes, SyntaxToken blameToken);

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
		void visit_throw_statement(const ThrowStatementSyntax* node) override;
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
