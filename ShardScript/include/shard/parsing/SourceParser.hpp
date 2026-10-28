#pragma once
#include <shard/Definitions.hpp>

#include <gmt/Arena.hpp>
#include <gmt/Containers.hpp>
#include <gmt/Ref.hpp>

#include <shard/parsing/Diagnostics.hpp>
#include <shard/parsing/SyntaxToken.hpp>
#include <shard/parsing/TokenType.hpp>
#include <shard/parsing/SyntaxNode.hpp>
#include <shard/parsing/SyntaxTree.hpp>

#include <shard/lexical/SourceProvider.hpp>

#include <shard/parsing/nodes/TranslationUnitSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarationSyntax.hpp>
#include <shard/parsing/nodes/TypeDeclarationSyntax.hpp>
#include <shard/parsing/nodes/BlockSyntax.hpp>
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

#include <shard/parsing/nodes/members/StructDeclarationSyntax.hpp>
#include <shard/parsing/nodes/members/InterfaceDeclarationSyntax.hpp>
#include <shard/parsing/nodes/members/EnumDeclarationSyntax.hpp>
#include <shard/parsing/nodes/members/EnumFieldDeclarationSyntax.hpp>
#include <shard/parsing/nodes/members/FieldDeclarationSyntax.hpp>
#include <shard/parsing/nodes/members/PropertyDeclarationSyntax.hpp>
#include <shard/parsing/nodes/members/IndexatorDeclarationSyntax.hpp>
#include <shard/parsing/nodes/members/AccessorDeclarationSyntax.hpp>
#include <shard/parsing/nodes/members/ConstructorDeclarationSyntax.hpp>
#include <shard/parsing/nodes/members/OperatorDeclarationSyntax.hpp>
#include <shard/parsing/nodes/members/DelegateDeclarationSyntax.hpp>
#include <shard/parsing/nodes/members/FunctionDeclarationSyntax.hpp>

#include <shard/parsing/nodes/statements/ExpressionStatementSyntax.hpp>
#include <shard/parsing/nodes/statements/VariableStatementSyntax.hpp>
#include <shard/parsing/nodes/statements/DeferStatementSyntax.hpp>
#include <shard/parsing/nodes/statements/DeferBlockSyntax.hpp>
#include <shard/parsing/nodes/statements/DeferVariableSyntax.hpp>
#include <shard/parsing/nodes/statements/ReturnStatementSyntax.hpp>
#include <shard/parsing/nodes/statements/BreakStatementSyntax.hpp>
#include <shard/parsing/nodes/statements/ContinueStatementSyntax.hpp>

#include <shard/parsing/nodes/statements/ConditionalClauseBaseSyntax.hpp>
#include <shard/parsing/nodes/statements/IfStatementSyntax.hpp>
#include <shard/parsing/nodes/statements/UnlessStatementSyntax.hpp>
#include <shard/parsing/nodes/statements/ElseStatementSyntax.hpp>
#include <shard/parsing/nodes/statements/ThrowStatementSyntax.hpp>
#include <shard/parsing/nodes/statements/TryStatementSyntax.hpp>
#include <shard/parsing/nodes/statements/CatchClauseSyntax.hpp>

#include <shard/parsing/nodes/loops/WhileStatementSyntax.hpp>
#include <shard/parsing/nodes/loops/UntilStatementSyntax.hpp>
#include <shard/parsing/nodes/loops/ForInStatementSyntax.hpp>

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

#include <shard/parsing/nodes/directives/UsingDirectiveSyntax.hpp>
#include <shard/parsing/nodes/directives/NamespaceDirectiveSyntax.hpp>

#include <algorithm>
#include <initializer_list>
#include <memory>
#include <memory_resource>
#include <vector>

namespace shard
{
	// Note that this parser is only capable of contextual parsing,
	// and only should be used to parse full translation units.
	// It is recomended not try to parse individual members or expression with this parser out of stream.

	class SHARD_API SourceParser
	{
		enum class ParseState : std::uint8_t
		{
			None
		};

		struct ParseFrame
		{
			SyntaxKind kind;
			ParseState state;
		};

		struct RecoveryContext
		{
			class Guard
			{
				SourceParser& m_parser;

			public:
				Guard(SourceParser& parser, SyntaxKind kind, ParseState state = ParseState::None);
				~Guard();

				Guard(const Guard&) = delete;
				Guard& operator=(const Guard&) = delete;

				void set_state(ParseState state);
			};

			gmt::vector<ParseFrame> parseStack;
		};

		static constexpr int max_loop_iterations = 10000;
		static constexpr int max_block_depth = 256;
		static constexpr int max_expression_depth = 256;

		SyntaxTree& m_syntaxTree;
		DiagnosticsContext& m_diagnostics;
		RecoveryContext m_recovery;
		int m_blockDepth = 0;
		int m_expressionDepth = 0;

	public:
		SourceParser(SyntaxTree& syntaxTree, DiagnosticsContext& diagnostics);
		~SourceParser();

		void FromSourceProvider(SourceProvider& reader);

	private:
		// Token stream helpers
		SyntaxToken expect(SourceProvider& reader, TokenType type, const wchar_t* message);
		bool matches(SourceProvider& reader, std::initializer_list<TokenType> types);
		bool try_match(SourceProvider& reader, std::initializer_list<TokenType> types, const wchar_t* errorMessage, int maxSkips = 5);
		bool try_match_identifier(SourceProvider& reader, int maxSkips = 5);
		int scan_type_argument_list_end(SourceProvider& reader);
		bool scan_generic_type_arguments(SourceProvider& reader);
		bool scan_generic_invokation(SourceProvider& reader);
		void reject_generic_type_parameters(SourceProvider& reader, const wchar_t* message);
		bool skip_towards(SourceProvider& reader, std::initializer_list<TokenType> expectedTokens, int maxSkips);
		bool try_synchronize(SourceProvider& reader);

		// Translation unit level
		TranslationUnitSyntax* read_compilation_unit(SourceProvider& reader);
		UsingDirectiveSyntax* read_using_directive(SourceProvider& reader, SyntaxNode* parent);
		NamespaceDirectiveSyntax* read_namespace_directive(SourceProvider& reader, SyntaxNode* parent);

		// Members
		MemberDeclarationSyntax* read_member_declaration(SourceProvider& reader, SyntaxNode* parent);
		StructDeclarationSyntax* read_struct_declaration(SourceProvider& reader, SyntaxNode* parent);
		InterfaceDeclarationSyntax* read_interface_declaration(SourceProvider& reader, SyntaxNode* parent);
		EnumDeclarationSyntax* read_enum_declaration(SourceProvider& reader, SyntaxNode* parent);
		EnumFieldDeclarationSyntax* read_enum_field_declaration(SourceProvider& reader, SyntaxNode* parent);
		FieldDeclarationSyntax* read_field_declaration(SourceProvider& reader, SyntaxNode* parent);
		PropertyDeclarationSyntax* read_property_declaration(SourceProvider& reader, SyntaxNode* parent);
		IndexatorDeclarationSyntax* read_indexator_declaration(SourceProvider& reader, SyntaxNode* parent);
		AccessorDeclarationSyntax* read_accessor_declaration(SourceProvider& reader, SyntaxNode* parent);
		ConstructorDeclarationSyntax* read_constructor_declaration(SourceProvider& reader, SyntaxNode* parent);
		OperatorDeclarationSyntax* read_operator_declaration(SourceProvider& reader, SyntaxNode* parent);
		DelegateDeclarationSyntax* read_delegate_declaration(SourceProvider& reader, SyntaxNode* parent);
		FunctionDeclarationSyntax* read_function_declaration(SourceProvider& reader, SyntaxNode* parent);

		// Types
		TypeSyntax* read_type(SourceProvider& reader, SyntaxNode* parent);
		TypeSyntax* read_identifier_name_type(SourceProvider& reader, SyntaxNode* parent);
		GenericTypeSyntax* read_generic_type(SourceProvider& reader, SyntaxNode* parent, TypeSyntax* underlayingType);

		// Parameters & Lists
		ParameterSyntax* read_parameter(SourceProvider& reader, SyntaxNode* parent);
		ParameterSyntax* read_lambda_parameter(SourceProvider& reader, SyntaxNode* parent);
		ParametersListSyntax* read_method_parameters(SourceProvider& reader, SyntaxNode* parent);
		ParametersListSyntax* read_indexer_parameters(SourceProvider& reader, SyntaxNode* parent);
		ParametersListSyntax* read_lambda_parameters(SourceProvider& reader, SyntaxNode* parent);

		TypeParameterSyntax* read_type_parameter(SourceProvider& reader, SyntaxNode* parent);
		TypeParametersListSyntax* read_generic_type_parameters(SourceProvider& reader, SyntaxNode* parent);
		TypeArgumentsListSyntax* read_type_arguments_list(SourceProvider& reader, SyntaxNode* parent);

		ArgumentSyntax* read_argument(SourceProvider& reader, SyntaxNode* parent);
		ArgumentsListSyntax* read_arguments_list(SourceProvider& reader, SyntaxNode* parent);

		WhereClauseSyntax* read_where_clause(SourceProvider& reader, SyntaxNode* parent);
		WhereClausesListSyntax* read_where_clauses(SourceProvider& reader, SyntaxNode* parent);
		BaseTypesListSyntax* read_base_types(SourceProvider& reader, SyntaxNode* parent);

		BlockSyntax* read_body(SourceProvider& reader, SyntaxNode* parent);
		StatementsBlockSyntax* read_statements_block(SourceProvider& reader, SyntaxNode* parent);
		ArrowClauseSyntax* read_arrow_clause(SourceProvider& reader, SyntaxNode* parent);
		StatementSyntax* read_statement(SourceProvider& reader, SyntaxNode* parent);
		ExpressionStatementSyntax* read_expression_statement(SourceProvider& reader, SyntaxNode* parent);
		VariableStatementSyntax* read_variable_statement(SourceProvider& reader, SyntaxNode* parent);
		StatementSyntax* read_defer(SourceProvider& reader, SyntaxNode* parent);
		DeferStatementSyntax* read_defer_statement(SourceProvider& reader, SyntaxNode* parent);
		DeferBlockSyntax* read_defer_block(SourceProvider& reader, SyntaxNode* parent);
		DeferVariableSyntax* read_defer_variable(SourceProvider& reader, SyntaxNode* parent);
		ReturnStatementSyntax* read_return_statement(SourceProvider& reader, SyntaxNode* parent);
		BreakStatementSyntax* read_break_statement(SourceProvider& reader, SyntaxNode* parent);
		ContinueStatementSyntax* read_continue_statement(SourceProvider& reader, SyntaxNode* parent);
		ConditionalClauseBaseSyntax* read_conditional_clause(SourceProvider& reader, SyntaxNode* parent);
		ThrowStatementSyntax* read_throw_statement(SourceProvider& reader, SyntaxNode* parent);
		TryStatementSyntax* read_try_statement(SourceProvider& reader, SyntaxNode* parent);
		CatchClauseSyntax* read_catch_clause(SourceProvider& reader, SyntaxNode* parent);
		WhileStatementSyntax* read_while_statement(SourceProvider& reader, SyntaxNode* parent);
		UntilStatementSyntax* read_until_statement(SourceProvider& reader, SyntaxNode* parent);
		ForInStatementSyntax* read_for_in_statement(SourceProvider& reader, SyntaxNode* parent);

		ExpressionSyntax* read_expression(SourceProvider& reader, SyntaxNode* parent, int parentPrecedence = 0);
		ExpressionSyntax* read_operand(SourceProvider& reader, SyntaxNode* parent);

		LiteralExpressionSyntax* read_literal_expression(SourceProvider& reader, SyntaxNode* parent);
		UnaryExpressionSyntax* read_unary_expression(SourceProvider& reader, SyntaxNode* parent);
		AwaitExpressionSyntax* read_await_expression(SourceProvider& reader, SyntaxNode* parent);
		IfExpressionSyntax* read_if_expression(SourceProvider& reader, SyntaxNode* parent);
		CollectionExpressionSyntax* read_collection_expression(SourceProvider& reader, SyntaxNode* parent);
		ObjectExpressionSyntax* read_object_expression(SourceProvider& reader, SyntaxNode* parent);
		LambdaExpressionSyntax* read_lambda_expression(SourceProvider& reader, SyntaxNode* parent);
		ExpressionSyntax* read_linked_expression(SourceProvider& reader, SyntaxNode* parent, ExpressionSyntax* previous);
		ArgumentsListSyntax* read_indexer_arguments_list(SourceProvider& reader, SyntaxNode* parent);
		InvokationExpressionSyntax* read_invokation_expression(SourceProvider& reader, SyntaxNode* parent, ExpressionSyntax* previous, const SyntaxToken& identifier, const SyntaxToken& delimeter, TypeArgumentsListSyntax* typeArguments = nullptr);

		AttributeSyntax* read_attribute(SourceProvider& reader, SyntaxNode* parent);
		AttributesListSyntax* read_attributes_list(SourceProvider& reader, SyntaxNode* parent);
		void read_member_modifiers(SourceProvider& reader, std::pmr::vector<SyntaxToken>& modifiers);
	};
}
