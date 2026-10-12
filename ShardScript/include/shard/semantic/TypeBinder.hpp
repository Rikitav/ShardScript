#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxVisitor.hpp>
#include <shard/semantic/SemanticModel.hpp>
#include <shard/semantic/SemanticVisitor.hpp>
#include <shard/semantic/SymbolFactory.hpp>

#include <shard/parsing/nodes/TranslationUnitSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarationSyntax.hpp>
#include <shard/parsing/nodes/TypeSyntax.hpp>

#include <shard/parsing/nodes/Directives/UsingDirectiveSyntax.hpp>

#include <shard/parsing/nodes/lists/ParametersListSyntax.hpp>
#include <shard/parsing/nodes/lists/ArgumentsListSyntax.hpp>
#include <shard/parsing/nodes/lists/WhereClausesListSyntax.hpp>

#include <shard/parsing/nodes/statements/ConditionalClauseSyntax.hpp>
#include <shard/parsing/nodes/statements/DeferStatementSyntax.hpp>
#include <shard/parsing/nodes/statements/ElseStatementSyntax.hpp>
#include <shard/parsing/nodes/statements/ExpressionStatementSyntax.hpp>
#include <shard/parsing/nodes/statements/IfStatementSyntax.hpp>
#include <shard/parsing/nodes/statements/ReturnStatementSyntax.hpp>
#include <shard/parsing/nodes/statements/ThrowStatementSyntax.hpp>
#include <shard/parsing/nodes/statements/TryStatementSyntax.hpp>
#include <shard/parsing/nodes/statements/UnlessStatementSyntax.hpp>
#include <shard/parsing/nodes/statements/VariableStatementSyntax.hpp>
#include <shard/parsing/nodes/expressions/LambdaExpressionSyntax.hpp>
#include <shard/parsing/nodes/expressions/ObjectExpressionSyntax.hpp>

#include <shard/parsing/nodes/loops/ForInStatementSyntax.hpp>
#include <shard/parsing/nodes/loops/UntilStatementSyntax.hpp>
#include <shard/parsing/nodes/loops/WhileStatementSyntax.hpp>

#include <shard/parsing/nodes/members/AccessorDeclarationSyntax.hpp>
#include <shard/parsing/nodes/members/ConstructorDeclarationSyntax.hpp>
#include <shard/parsing/nodes/members/DelegateDeclarationSyntax.hpp>
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

#include <string_view>
#include <unordered_set>
#include <vector>

namespace shard
{
	class SHARD_API TypeBinder final : public SemanticVisitor
	{
		SymbolFactory m_factory;
		std::unordered_set<std::wstring_view> m_ambiguousNames;

		void import_namespace(gmt::Ref<const UsingDirectiveSyntax> node, gmt::Ref<NamespaceSymbol> namespaceSymbol);
		bool is_ambiguous_name(std::wstring_view name);

		gmt::Ref<NamespaceSymbol> resolve_namespace_type(gmt::Ref<const TypeSyntax> type);
		gmt::Ref<StructSymbol> resolve_type(gmt::Ref<const TypeSyntax> node);

		bool type_satisfies_constraint(gmt::Ref<StructSymbol> type_argument, gmt::Ref<StructSymbol> constraint);
		bool type_implements_interface(gmt::Ref<StructSymbol> type_argument, gmt::Ref<InterfaceSymbol> interface_symbol);

		void bind_expression(gmt::Ref<const ExpressionSyntax> expression);
		void bind_arguments(gmt::Ref<const ArgumentsListSyntax> arguments);

		void bind_where_clauses(gmt::Ref<const WhereClausesListSyntax> clauses, const std::vector<gmt::Ref<TypeParameterSymbol>>& type_parameters);
		void bind_base_types(gmt::Ref<StructSymbol> symbol, gmt::Ref<const BaseTypesListSyntax> base_types);

	public:
		TypeBinder(SemanticModel& model, DiagnosticsContext& diagnostics);

		void visit_translation_unit(const TranslationUnitSyntax* node) override;
		void visit_using_directive(const UsingDirectiveSyntax* node) override;

		void visit_struct_declaration(const StructDeclarationSyntax* node) override;
		void visit_interface_declaration(const InterfaceDeclarationSyntax* node) override;
		void visit_delegate_declaration(const DelegateDeclarationSyntax* node) override;
		void visit_enum_declaration(const EnumDeclarationSyntax* node) override;

		void visit_constructor_declaration(const ConstructorDeclarationSyntax* node) override;
		void visit_function_declaration(const FunctionDeclarationSyntax* node) override;
		void visit_operator_declaration(const OperatorDeclarationSyntax* node) override;
		void visit_field_declaration(const FieldDeclarationSyntax* node) override;
		void visit_property_declaration(const PropertyDeclarationSyntax* node) override;
		void visit_indexator_declaration(const IndexatorDeclarationSyntax* node) override;
		void visit_accessor_declaration(const AccessorDeclarationSyntax* node) override;

		void visit_variable_statement(const VariableStatementSyntax* node) override;
		void visit_try_statement(const TryStatementSyntax* node) override;
		void visit_lambda_expression(const LambdaExpressionSyntax* node) override;
		void visit_object_creation_expression(const ObjectExpressionSyntax* node) override;

		void visit_expression_statement(const ExpressionStatementSyntax* node) override;
		void visit_return_statement(const ReturnStatementSyntax* node) override;
		void visit_throw_statement(const ThrowStatementSyntax* node) override;
		void visit_defer_statement(const DeferStatementSyntax* node) override;
		void visit_if_statement(const IfStatementSyntax* node) override;
		void visit_unless_statement(const UnlessStatementSyntax* node) override;
		void visit_else_statement(const ElseStatementSyntax* node) override;
		void visit_while_statement(const WhileStatementSyntax* node) override;
		void visit_until_statement(const UntilStatementSyntax* node) override;
		void visit_for_in_statement(const ForInStatementSyntax* node) override;
		void visit_statements_block(const StatementsBlockSyntax* node) override;

		void visit_parameter(const ParameterSyntax* node) override;

		void visit_predefined_type(const PredefinedTypeSyntax* node) override;
		void visit_identifier_name_type(const IdentifierNameTypeSyntax* node) override;
		void visit_qualified_name_type(const QualifiedNameTypeSyntax* node) override;
		void visit_array_type(const ArrayTypeSyntax* node) override;
		void visit_nullable_type(const NullableTypeSyntax* node) override;
		void visit_generic_type(const GenericTypeSyntax* node) override;
	};
}
