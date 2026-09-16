#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxToken.hpp>
#include <shard/parsing/TokenType.hpp>
#include <shard/parsing/SyntaxNode.hpp>
#include <shard/parsing/SyntaxTree.hpp>
#include <gmt/Arena.hpp>
#include <gmt/Ref.hpp>
#include <shard/parsing/Diagnostics.hpp>

#include <shard/lexical/SourceProvider.hpp>

#include <shard/parsing/nodes/TranslationUnitSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarationSyntax.hpp>
#include <shard/parsing/nodes/TypeDeclarationSyntax.hpp>
#include <shard/parsing/nodes/BodySyntax.hpp>
#include <shard/parsing/nodes/StatementSyntax.hpp>
#include <shard/parsing/nodes/ExpressionSyntax.hpp>
#include <shard/parsing/nodes/TypeSyntax.hpp>

#include <shard/parsing/nodes/lists/AttributesListSyntax.hpp>
#include <shard/parsing/nodes/lists/ParametersListSyntax.hpp>
#include <shard/parsing/nodes/lists/ArgumentsListSyntax.hpp>
#include <shard/parsing/nodes/lists/TypeArgumentsListSyntax.hpp>
#include <shard/parsing/nodes/lists/TypeParametersListSyntax.hpp>
#include <shard/parsing/nodes/lists/BaseTypesListSyntax.hpp>
#include <shard/parsing/nodes/lists/WhereClausesListSyntax.hpp>

#include <shard/parsing/nodes/blocks/StatementsBlockSyntax.hpp>
#include <shard/parsing/nodes/blocks/ArrowClauseSyntax.hpp>

#include <shard/parsing/nodes/members/ClassDeclarationSyntax.hpp>
#include <shard/parsing/nodes/members/FunctionDeclarationSyntax.hpp>

#include <shard/parsing/nodes/statements/ExpressionStatementSyntax.hpp>
#include <shard/parsing/nodes/statements/VariableStatementSyntax.hpp>
#include <shard/parsing/nodes/statements/DeferStatementSyntax.hpp>
#include <shard/parsing/nodes/statements/DeferBlockSyntax.hpp>
#include <shard/parsing/nodes/statements/DeferVariableSyntax.hpp>

#include <shard/parsing/nodes/expressions/LiteralExpressionsSyntax.hpp>
#include <shard/parsing/nodes/expressions/BinaryExpressionSyntax.hpp>
#include <shard/parsing/nodes/expressions/MemberAccessExpressionSyntax.hpp>
#include <shard/parsing/nodes/expressions/InvokationExpressionSyntax.hpp>
#include <shard/parsing/nodes/expressions/IndexatorExpressionSyntax.hpp>
#include <shard/parsing/nodes/expressions/UnaryExpressionSyntax.hpp>
#include <shard/parsing/nodes/expressions/IsExpressionSyntax.hpp>
#include <shard/parsing/nodes/expressions/CastExpressionSyntax.hpp>
#include <shard/parsing/nodes/expressions/IfExpressionSyntax.hpp>
#include <shard/parsing/nodes/expressions/AwaitExpressionSyntax.hpp>
#include <shard/parsing/nodes/expressions/RangeExpressionSyntax.hpp>
#include <shard/parsing/nodes/expressions/CollectionExpressionSyntax.hpp>
#include <shard/parsing/nodes/expressions/ObjectExpressionSyntax.hpp>
#include <shard/parsing/nodes/expressions/LambdaExpressionSyntax.hpp>

#include <shard/parsing/nodes/types/PredefinedTypeSyntax.hpp>
#include <shard/parsing/nodes/types/IdentifierNameTypeSyntax.hpp>
#include <shard/parsing/nodes/types/QualifiedNameTypeSyntax.hpp>
#include <shard/parsing/nodes/types/GenericTypeSyntax.hpp>
#include <shard/parsing/nodes/types/ArrayTypeSyntax.hpp>
#include <shard/parsing/nodes/types/NullableTypeSyntax.hpp>

#include <shard/parsing/nodes/Directives/UsingDirectiveSyntax.hpp>
#include <shard/parsing/nodes/Directives/NamespaceDirectiveSyntax.hpp>

#include <algorithm>
#include <initializer_list>
#include <memory_resource>
#include <vector>

namespace shard
{
	// Note that this parser is only capable of contextual parsing, and only should be used to parse full compulation units. DO NOT try to parse individual members or expression with this parser out of stream

	class SHARD_API SourceParser
	{
		static constexpr int max_loop_iterations = 10000;
		static constexpr int max_block_depth = 256;
		static constexpr int max_expression_depth = 256;

		SyntaxTree& m_syntaxTree;
		DiagnosticsContext& m_diagnostics;
		int m_blockDepth = 0;
		int m_expressionDepth = 0;

	public:
		SourceParser(SyntaxTree& syntaxTree, DiagnosticsContext& diagnostics);
		~SourceParser() = default;

		void FromSourceProvider(SourceProvider& reader);

	private:
		// Token stream helpers
		SyntaxToken expect(SourceProvider& reader, TokenType type, const wchar_t* message);
		bool matches(SourceProvider& reader, std::initializer_list<TokenType> types);
		bool try_match(SourceProvider& reader, std::initializer_list<TokenType> types, const wchar_t* errorMessage, int maxSkips = 5);
		bool try_match_identifier(SourceProvider& reader, int maxSkips = 5);
		bool scan_generic_type_arguments(SourceProvider& reader);

		// Translation unit level
		gmt::Ref<TranslationUnitSyntax> read_compilation_unit(SourceProvider& reader);
		gmt::Ref<UsingDirectiveSyntax> read_using_directive(SourceProvider& reader, gmt::Ref<SyntaxNode> parent);
		gmt::Ref<NamespaceDirectiveSyntax> read_namespace_directive(SourceProvider& reader, gmt::Ref<SyntaxNode> parent);

		// Members
		gmt::Ref<MemberDeclarationSyntax> read_member_declaration(SourceProvider& reader, gmt::Ref<SyntaxNode> parent);
		gmt::Ref<ClassDeclarationSyntax> read_class_declaration(SourceProvider& reader, gmt::Ref<SyntaxNode> parent);
		gmt::Ref<FunctionDeclarationSyntax> read_function_declaration(SourceProvider& reader, gmt::Ref<SyntaxNode> parent);

		// Types
		gmt::Ref<TypeSyntax> read_type(SourceProvider& reader, gmt::Ref<SyntaxNode> parent);
		gmt::Ref<TypeSyntax> read_identifier_name_type(SourceProvider& reader, gmt::Ref<SyntaxNode> parent);
		gmt::Ref<GenericTypeSyntax> read_generic_type(SourceProvider& reader, gmt::Ref<SyntaxNode> parent, gmt::Ref<TypeSyntax> underlayingType);

		// Parameters & Lists
		gmt::Ref<ParameterSyntax> read_parameter(SourceProvider& reader, gmt::Ref<SyntaxNode> parent);
		gmt::Ref<ParametersListSyntax> read_method_parameters(SourceProvider& reader, gmt::Ref<SyntaxNode> parent);
		gmt::Ref<ParametersListSyntax> read_indexer_parameters(SourceProvider& reader, gmt::Ref<SyntaxNode> parent);
		gmt::Ref<ParametersListSyntax> read_lambda_parameters(SourceProvider& reader, gmt::Ref<SyntaxNode> parent);

		gmt::Ref<TypeParameterSyntax> read_type_parameter(SourceProvider& reader, gmt::Ref<SyntaxNode> parent);
		gmt::Ref<TypeParametersListSyntax> read_generic_type_parameters(SourceProvider& reader, gmt::Ref<SyntaxNode> parent);
		gmt::Ref<TypeArgumentsListSyntax> read_type_arguments_list(SourceProvider& reader, gmt::Ref<SyntaxNode> parent);

		gmt::Ref<ArgumentSyntax> read_argument(SourceProvider& reader, gmt::Ref<SyntaxNode> parent);
		gmt::Ref<ArgumentsListSyntax> read_arguments_list(SourceProvider& reader, gmt::Ref<SyntaxNode> parent);

		gmt::Ref<WhereClauseSyntax> read_where_clause(SourceProvider& reader, gmt::Ref<SyntaxNode> parent);
		gmt::Ref<WhereClausesListSyntax> read_where_clauses(SourceProvider& reader, gmt::Ref<SyntaxNode> parent);
		gmt::Ref<BaseTypesListSyntax> read_base_types(SourceProvider& reader, gmt::Ref<SyntaxNode> parent);

		gmt::Ref<BodySyntax> read_body(SourceProvider& reader, gmt::Ref<SyntaxNode> parent);
		gmt::Ref<StatementsBlockSyntax> read_statements_block(SourceProvider& reader, gmt::Ref<SyntaxNode> parent);
		gmt::Ref<ArrowClauseSyntax> read_arrow_clause(SourceProvider& reader, gmt::Ref<SyntaxNode> parent);
		gmt::Ref<StatementSyntax> read_statement(SourceProvider& reader, gmt::Ref<SyntaxNode> parent);
		gmt::Ref<ExpressionStatementSyntax> read_expression_statement(SourceProvider& reader, gmt::Ref<SyntaxNode> parent);
		gmt::Ref<VariableStatementSyntax> read_variable_statement(SourceProvider& reader, gmt::Ref<SyntaxNode> parent);
		gmt::Ref<StatementSyntax> read_defer(SourceProvider& reader, gmt::Ref<SyntaxNode> parent);
		gmt::Ref<DeferStatementSyntax> read_defer_statement(SourceProvider& reader, gmt::Ref<SyntaxNode> parent);
		gmt::Ref<DeferBlockSyntax> read_defer_block(SourceProvider& reader, gmt::Ref<SyntaxNode> parent);
		gmt::Ref<DeferVariableSyntax> read_defer_variable(SourceProvider& reader, gmt::Ref<SyntaxNode> parent);

		gmt::Ref<ExpressionSyntax> read_expression(SourceProvider& reader, gmt::Ref<SyntaxNode> parent, int parentPrecedence = 0);
		gmt::Ref<ExpressionSyntax> read_operand(SourceProvider& reader, gmt::Ref<SyntaxNode> parent);

		gmt::Ref<LiteralExpressionSyntax> read_literal_expression(SourceProvider& reader, gmt::Ref<SyntaxNode> parent);
		gmt::Ref<UnaryExpressionSyntax> read_unary_expression(SourceProvider& reader, gmt::Ref<SyntaxNode> parent);
		gmt::Ref<AwaitExpressionSyntax> read_await_expression(SourceProvider& reader, gmt::Ref<SyntaxNode> parent);
		gmt::Ref<IfExpressionSyntax> read_if_expression(SourceProvider& reader, gmt::Ref<SyntaxNode> parent);
		gmt::Ref<CollectionExpressionSyntax> read_collection_expression(SourceProvider& reader, gmt::Ref<SyntaxNode> parent);
		gmt::Ref<ObjectExpressionSyntax> read_object_expression(SourceProvider& reader, gmt::Ref<SyntaxNode> parent);
		gmt::Ref<LambdaExpressionSyntax> read_lambda_expression(SourceProvider& reader, gmt::Ref<SyntaxNode> parent);
		gmt::Ref<ExpressionSyntax> read_linked_expression(SourceProvider& reader, gmt::Ref<SyntaxNode> parent, gmt::Ref<ExpressionSyntax> previous);
		gmt::Ref<ArgumentsListSyntax> read_indexer_arguments_list(SourceProvider& reader, gmt::Ref<SyntaxNode> parent);
		gmt::Ref<InvokationExpressionSyntax> read_invokation_expression(SourceProvider& reader, gmt::Ref<SyntaxNode> parent, gmt::Ref<ExpressionSyntax> previous, const SyntaxToken& identifier, const SyntaxToken& delimeter);

		gmt::Ref<AttributeSyntax> read_attribute(SourceProvider& reader, gmt::Ref<SyntaxNode> parent);
		gmt::Ref<AttributesListSyntax> read_attributes_list(SourceProvider& reader, gmt::Ref<SyntaxNode> parent);
		void read_member_modifiers(SourceProvider& reader, std::pmr::vector<SyntaxToken>& modifiers);
	};
}
