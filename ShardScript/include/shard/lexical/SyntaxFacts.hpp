#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/TokenType.hpp>
#include <shard/parsing/SyntaxKind.hpp>

#include <string>

namespace shard
{
	SHARD_API bool is_synchronization_token(TokenType type);
	SHARD_API bool can_start_member_declaration(TokenType type);
	SHARD_API bool can_start_compilation_unit(TokenType type);

	SHARD_API bool is_punctuation(shard::TokenType type);
	SHARD_API bool is_reserved_identifier(shard::TokenType type);

	SHARD_API int get_operator_precendence(shard::TokenType type);
	SHARD_API bool is_operator(shard::TokenType type);

	SHARD_API bool is_binary_operator(shard::TokenType type);
	SHARD_API bool is_binary_arithmetic_operator(shard::TokenType type);
	SHARD_API bool is_binary_boolean_operator(shard::TokenType type);
	SHARD_API bool is_binary_bit_operator(shard::TokenType type);

	SHARD_API bool is_unary_operator(shard::TokenType type);
	SHARD_API bool is_right_unary_operator(shard::TokenType type);
	SHARD_API bool is_right_unary_arithmetic_operator(shard::TokenType type);
	SHARD_API bool is_right_unary_boolean_operator(shard::TokenType type);

	SHARD_API bool is_left_unary_operator(shard::TokenType type);
	SHARD_API bool is_left_unary_arithmetic_operator(shard::TokenType type);
	SHARD_API bool is_left_unary_boolean_operator(shard::TokenType type);

	SHARD_API bool is_modifier(shard::TokenType type);
	SHARD_API bool is_type_keyword(shard::TokenType type);
	SHARD_API bool is_member_keyword(shard::TokenType type);
	SHARD_API bool is_member_declaration(shard::TokenType currentType, shard::TokenType peekType);

	SHARD_API bool is_predefined_type(shard::TokenType type);
	SHARD_API bool is_type(shard::TokenType type, shard::TokenType peekType);

	SHARD_API bool is_keyword(shard::TokenType type);
	SHARD_API bool is_look_keyword(shard::TokenType type);
	SHARD_API bool is_conditional_keyword(shard::TokenType type);
	SHARD_API bool is_functional_keyword(shard::TokenType type);

	SHARD_API bool is_linked_expression_node(shard::SyntaxKind kind);

	SHARD_API bool is_overloadable_operator(shard::TokenType type);
	SHARD_API std::wstring get_operator_method_name(shard::TokenType type);
	SHARD_API shard::TokenType get_operator_type(const std::wstring& name);
}
