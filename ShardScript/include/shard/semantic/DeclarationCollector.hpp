#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxVisitor.hpp>
#include <shard/semantic/SemanticModel.hpp>
#include <shard/semantic/SemanticVisitor.hpp>
#include <shard/semantic/SymbolFactory.hpp>

#include <shard/parsing/nodes/TranslationUnitSyntax.hpp>
#include <shard/parsing/nodes/lists/TypeParametersListSyntax.hpp>
#include <shard/parsing/nodes/loops/ForInStatementSyntax.hpp>
#include <shard/parsing/nodes/statements/TryStatementSyntax.hpp>
#include <shard/parsing/nodes/statements/VariableStatementSyntax.hpp>
#include <shard/parsing/nodes/expressions/LambdaExpressionSyntax.hpp>
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

#include <cstdint>

namespace shard
{
	class SHARD_API DeclarationCollector final : public SemanticVisitor
	{
		SymbolFactory m_factory;

		void collect_type_parameters(gmt::Ref<StructSymbol> owner, gmt::Ref<const TypeParametersListSyntax> list, std::uint16_t ownerParameterOffset = 0);
		void collect_type_parameters(gmt::Ref<MethodSymbol> owner, gmt::Ref<const TypeParametersListSyntax> list, std::uint16_t ownerParameterOffset = 0);

		gmt::Ref<SyntaxSymbol> get_declaration_parent();

	public:
		DeclarationCollector(SemanticModel& model, DiagnosticsContext& diagnostics);

		void visit_translation_unit(const TranslationUnitSyntax* node) override;

		void visit_struct_declaration(const StructDeclarationSyntax* node) override;
		void visit_interface_declaration(const InterfaceDeclarationSyntax* node) override;
		void visit_delegate_declaration(const DelegateDeclarationSyntax* node) override;
		void visit_enum_declaration(const EnumDeclarationSyntax* node) override;

		void visit_function_declaration(const FunctionDeclarationSyntax* node) override;
		void visit_operator_declaration(const OperatorDeclarationSyntax* node) override;
		void visit_constructor_declaration(const ConstructorDeclarationSyntax* node) override;
		void visit_field_declaration(const FieldDeclarationSyntax* node) override;
		void visit_property_declaration(const PropertyDeclarationSyntax* node) override;
		void visit_indexator_declaration(const IndexatorDeclarationSyntax* node) override;
		void visit_accessor_declaration(const AccessorDeclarationSyntax* node) override;

		void visit_variable_statement(const VariableStatementSyntax* node) override;
		void visit_for_in_statement(const ForInStatementSyntax* node) override;
		void visit_try_statement(const TryStatementSyntax* node) override;
		void visit_lambda_expression(const LambdaExpressionSyntax* node) override;
		void visit_statements_block(const StatementsBlockSyntax* node) override;
	};
}