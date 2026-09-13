#include <shard/lexical/SyntaxFacts.hpp>
#include <shard/parsing/TokenType.hpp>
#include <shard/parsing/SyntaxKind.hpp>

using namespace shard;

bool shard::is_synchronization_token(TokenType type)
{
	switch (type)
	{
		case TokenType::Semicolon:
		case TokenType::OpenBrace:
		case TokenType::CloseBrace:
		case TokenType::EndOfFile:
			return true;

		default:
			return false;
	}
}

bool shard::can_start_member_declaration(TokenType type)
{
	return type == TokenType::OpenSquare
		|| type == TokenType::Identifier
		|| is_modifier(type)
		|| is_member_keyword(type);
}

bool shard::can_start_compilation_unit(TokenType type)
{
	return type == TokenType::UsingKeyword
		|| type == TokenType::NamespaceKeyword
		|| can_start_member_declaration(type);
}

int shard::get_operator_precendence(TokenType type)
{
	switch (type)
	{
		case TokenType::OpenCurl:
			return 12;

		case TokenType::IncrementOperator:
		case TokenType::DecrementOperator:
		case TokenType::AwaitKeyword:
			return 11;

		case TokenType::PowOperator:
			return 10;

		case TokenType::MultOperator:
		case TokenType::DivOperator:
		case TokenType::ModOperator:
			return 9;

		case TokenType::AddOperator:
		case TokenType::SubOperator:
			return 8;

		case TokenType::LeftShiftOperator:
		case TokenType::RightShiftOperator:
			return 7;

		case TokenType::RangeOperator:
		case TokenType::RangeInclusiveOperator:
			return 7;

		case TokenType::LessOperator:
		case TokenType::LessOrEqualsOperator:
		case TokenType::GreaterOperator:
		case TokenType::GreaterOrEqualsOperator:
			return 6;

		case TokenType::EqualsOperator:
		case TokenType::NotEqualsOperator:
		case TokenType::IsOperator:
		case TokenType::AsOperator:
			return 5;

		case TokenType::AndOperator:
			return 4;

		case TokenType::OrOperator:
			return 3;

		case TokenType::NullCoalescingOperator:
			return 2;

		case TokenType::Question: // ternary
			return 2;

		case TokenType::AssignOperator:
		case TokenType::AddAssignOperator:
		case TokenType::SubAssignOperator:
		case TokenType::MultAssignOperator:
		case TokenType::DivAssignOperator:
		case TokenType::ModAssignOperator:
		case TokenType::PowAssignOperator:
		case TokenType::OrAssignOperator:
		case TokenType::AndAssignOperator:
			return 1;

		default:
			return 0;
	}
}

bool shard::is_binary_arithmetic_operator(shard::TokenType type)
{
	switch (type)
	{
		case TokenType::AddOperator:
		case TokenType::SubOperator:
		case TokenType::MultOperator:
		case TokenType::DivOperator:
		case TokenType::ModOperator:
		case TokenType::PowOperator:
		case TokenType::AddAssignOperator:
		case TokenType::SubAssignOperator:
		case TokenType::MultAssignOperator:
		case TokenType::DivAssignOperator:
		case TokenType::ModAssignOperator:
		case TokenType::PowAssignOperator:
			return true;

		default:
			return false;
	}
}

bool shard::is_binary_boolean_operator(shard::TokenType type)
{
	switch (type)
	{
		case TokenType::LessOperator:
		case TokenType::LessOrEqualsOperator:
		case TokenType::GreaterOperator:
		case TokenType::GreaterOrEqualsOperator:
		case TokenType::EqualsOperator:
		case TokenType::NotEqualsOperator:
		case TokenType::AndOperator:
		case TokenType::OrOperator:
			return true;

		default:
			return false;
	}
}

bool shard::is_binary_bit_operator(shard::TokenType type)
{
	switch (type)
	{
		case TokenType::LeftShiftOperator:
		case TokenType::RightShiftOperator:
			return true;

		default:
			return false;
	}
}

bool shard::is_binary_operator(shard::TokenType type)
{
	if (is_binary_arithmetic_operator(type))
		return true;

	if (is_binary_boolean_operator(type))
		return true;

	if (is_binary_bit_operator(type))
		return true;

	if (type == TokenType::NullCoalescingOperator)
		return true;

	if (type == TokenType::RangeOperator)
		return true;

	if (type == TokenType::RangeInclusiveOperator)
		return true;

	if (type == TokenType::IsOperator)
		return true;

	if (type == TokenType::AsOperator)
		return true;

	return false;
}

bool shard::is_right_unary_arithmetic_operator(shard::TokenType type)
{
	switch (type)
	{
		case TokenType::IncrementOperator:
		case TokenType::DecrementOperator:
			return true;

		default:
			return false;
	}
}

bool shard::is_right_unary_boolean_operator(shard::TokenType type)
{
	switch (type)
	{
		case TokenType::NotOperator:
			return true;

		default:
			return false;
	}
}

bool shard::is_left_unary_operator(shard::TokenType type)
{
	if (is_left_unary_arithmetic_operator(type))
		return true;

	if (is_left_unary_boolean_operator(type))
		return true;

	return false;
}

bool shard::is_left_unary_arithmetic_operator(shard::TokenType type)
{
	switch (type)
	{
		case TokenType::SubOperator:
		case TokenType::AddOperator:
		case TokenType::IncrementOperator:
		case TokenType::DecrementOperator:
			return true;

		default:
			return false;
	}
}

bool shard::is_left_unary_boolean_operator(shard::TokenType type)
{
	return false;
}

bool shard::is_unary_operator(shard::TokenType type)
{
	if (is_left_unary_operator(type))
		return true;

	if (is_right_unary_operator(type))
		return true;

	return false;
}

bool shard::is_right_unary_operator(shard::TokenType type)
{
	if (is_right_unary_arithmetic_operator(type))
		return true;

	if (is_right_unary_boolean_operator(type))
		return true;

	return false;
}

bool shard::is_operator(shard::TokenType type)
{
	if (type == TokenType::AssignOperator)
		return true;

	if (is_unary_operator(type))
		return true;

	if (is_binary_operator(type))
		return true;

	return false;
}

bool shard::is_member_keyword(shard::TokenType type)
{
	switch (type)
	{
		case TokenType::IndexerKeyword:
			return true;

		default:
			return false;
	}
}

bool shard::is_predefined_type(shard::TokenType type)
{
	switch (type)
	{
		case TokenType::VoidKeyword:
		case TokenType::VarKeyword:
		case TokenType::IntegerKeyword:
		case TokenType::DoubleKeyword:
		case TokenType::ByteKeyword:
		case TokenType::NativeIntegerKeyword:
		case TokenType::CharKeyword:
		case TokenType::StringKeyword:
		case TokenType::BooleanKeyword:
			return true;

		default:
			return false;
	}
}

bool shard::is_type(shard::TokenType type, shard::TokenType peekType)
{
	if (is_predefined_type(type))
		return true;

	switch (type)
	{
		case TokenType::Identifier:
		{
			switch (peekType)
			{
				case TokenType::LessOperator:
				case TokenType::OpenSquare:
				case TokenType::Identifier:
				case TokenType::Question:
				case TokenType::IndexerKeyword:
					return true;

				default:
					return false;
			}
		}

		case TokenType::DelegateKeyword:
			return true;
	}

	return false;
}

bool shard::is_modifier(shard::TokenType type)
{
	switch (type)
	{
		case TokenType::PublicKeyword:
		case TokenType::PrivateKeyword:
		case TokenType::ProtectedKeyword:
		case TokenType::InternalKeyword:
		case TokenType::StaticKeyword:
		case TokenType::AsyncKeyword:
			return true;

		default:
			return false;
	}
}

bool shard::is_type_keyword(shard::TokenType type)
{
	switch (type)
	{
		case TokenType::ClassKeyword:
		case TokenType::StructKeyword:
		case TokenType::InterfaceKeyword:
		case TokenType::DelegateKeyword:
		case TokenType::EnumKeyword:
			return true;

		default:
			return false;
	}
}

bool shard::is_member_declaration(shard::TokenType currentType, shard::TokenType peekType)
{
	if (is_modifier(currentType))
		return true;

	if (is_member_keyword(currentType))
		return true;

	if (is_type_keyword(currentType))
		return true;

	if (currentType == TokenType::FunctionKeyword)
		return true;

	if (currentType == TokenType::InitKeyword)
		return true;

	if (currentType == TokenType::Identifier)
		return peekType == TokenType::Colon;

	if (currentType == TokenType::OpenSquare)
		return true;

	return false;
}

bool shard::is_look_keyword(shard::TokenType type)
{
	switch (type)
	{
		case TokenType::ForKeyword:
		case TokenType::WhileKeyword:
		case TokenType::UntilKeyword:
		case TokenType::DoKeyword:
		case TokenType::ForeachKeyword:
			return true;

		default:
			return false;
	}
}

bool shard::is_conditional_keyword(shard::TokenType type)
{
	switch (type)
	{
		case TokenType::IfKeyword:
		case TokenType::UnlessKeyword:
		case TokenType::ElseKeyword:
		case TokenType::SwitchKeyword:
			return true;

		default:
			return false;
	}
}

bool shard::is_functional_keyword(shard::TokenType type)
{
	switch (type)
	{
		case TokenType::BreakKeyword:
		case TokenType::ContinueKeyword:
		case TokenType::ReturnKeyword:
		case TokenType::ThrowKeyword:
		case TokenType::TryKeyword:
		case TokenType::CatchKeyword:
		case TokenType::DeferKeyword:
			return true;

		default:
			return false;
	}
}

bool shard::is_linked_expression_node(shard::SyntaxKind kind)
{
	switch (kind)
	{
		case SyntaxKind::MemberAccessExpression:
		case SyntaxKind::InvokationExpression:
		case SyntaxKind::IndexatorExpression:
			return true;

		default:
			return false;
	}
}

bool shard::is_keyword(shard::TokenType type)
{
	if (is_functional_keyword(type))
		return true;

	if (is_conditional_keyword(type))
		return true;

	if (is_look_keyword(type))
		return true;

	return false;
}

bool shard::is_punctuation(shard::TokenType type)
{
	switch (type)
	{
		case TokenType::OpenBrace:
		case TokenType::CloseBrace:
		case TokenType::OpenCurl:
		case TokenType::CloseCurl:
		case TokenType::OpenSquare:
		case TokenType::CloseSquare:
		case TokenType::Question:
		case TokenType::Delimeter:
		case TokenType::Comma:
		case TokenType::Colon:
		case TokenType::Semicolon:
			return true;

		default:
			return false;
	}
}

bool shard::is_reserved_identifier(shard::TokenType type)
{
	switch (type)
	{
		case TokenType::ValueKeyword:
		case TokenType::FieldKeyword:
		//case TokenType::ThisKeyword:
		case TokenType::WhileKeyword:
			return true;

		default:
			return false;
	}
}

std::wstring shard::get_operator_method_name(shard::TokenType type)
{
	switch (type)
	{
		case TokenType::AddOperator: return L"op_AddOperator";
		case TokenType::SubOperator: return L"op_SubOperator";
		case TokenType::MultOperator: return L"op_MultOperator";
		case TokenType::DivOperator: return L"op_DivOperator";
		case TokenType::ModOperator: return L"op_ModOperator";
		case TokenType::PowOperator: return L"op_PowOperator";

		case TokenType::AddAssignOperator: return L"op_AddAssignOperator";
		case TokenType::SubAssignOperator: return L"op_SubAssignOperator";
		case TokenType::MultAssignOperator: return L"op_MultAssignOperator";
		case TokenType::DivAssignOperator: return L"op_DivAssignOperator";
		case TokenType::ModAssignOperator: return L"op_ModAssignOperator";
		case TokenType::PowAssignOperator: return L"op_PowAssignOperator";

		case TokenType::OrOperator: return L"op_OrOperator";
		case TokenType::AndOperator: return L"op_AndOperator";
		case TokenType::OrAssignOperator: return L"op_OrAssignOperator";
		case TokenType::AndAssignOperator: return L"op_AndAssignOperator";
		case TokenType::RightShiftOperator: return L"op_RightShiftOperator";
		case TokenType::LeftShiftOperator: return L"op_LeftShiftOperator";

		case TokenType::EqualsOperator: return L"op_EqualsOperator";
		case TokenType::NotEqualsOperator: return L"op_NotEqualsOperator";
		case TokenType::GreaterOperator: return L"op_GreaterOperator";
		case TokenType::GreaterOrEqualsOperator: return L"op_GreaterOrEqualsOperator";
		case TokenType::LessOperator: return L"op_LessOperator";
		case TokenType::LessOrEqualsOperator: return L"op_LessOrEqualsOperator";

		case TokenType::IncrementOperator: return L"op_IncrementOperator";
		case TokenType::DecrementOperator: return L"op_DecrementOperator";
		case TokenType::NotOperator: return L"op_NotOperator";

		case TokenType::Delimeter: return L"op_DotOperator";
		case TokenType::AsOperator: return L"op_AsOperator";

		default: return L"";
	}
}

bool shard::is_overloadable_operator(shard::TokenType type)
{
	return !get_operator_method_name(type).empty();
}

shard::TokenType shard::get_operator_type(const std::wstring& name)
{
	if (name == L"AddOperator") return TokenType::AddOperator;
	if (name == L"SubOperator") return TokenType::SubOperator;
	if (name == L"MultOperator") return TokenType::MultOperator;
	if (name == L"DivOperator") return TokenType::DivOperator;
	if (name == L"ModOperator") return TokenType::ModOperator;
	if (name == L"PowOperator") return TokenType::PowOperator;

	if (name == L"AddAssignOperator") return TokenType::AddAssignOperator;
	if (name == L"SubAssignOperator") return TokenType::SubAssignOperator;
	if (name == L"MultAssignOperator") return TokenType::MultAssignOperator;
	if (name == L"DivAssignOperator") return TokenType::DivAssignOperator;
	if (name == L"ModAssignOperator") return TokenType::ModAssignOperator;
	if (name == L"PowAssignOperator") return TokenType::PowAssignOperator;

	if (name == L"OrOperator") return TokenType::OrOperator;
	if (name == L"AndOperator") return TokenType::AndOperator;
	if (name == L"OrAssignOperator") return TokenType::OrAssignOperator;
	if (name == L"AndAssignOperator") return TokenType::AndAssignOperator;
	if (name == L"RightShiftOperator") return TokenType::RightShiftOperator;
	if (name == L"LeftShiftOperator") return TokenType::LeftShiftOperator;

	if (name == L"EqualsOperator") return TokenType::EqualsOperator;
	if (name == L"NotEqualsOperator") return TokenType::NotEqualsOperator;
	if (name == L"GreaterOperator") return TokenType::GreaterOperator;
	if (name == L"GreaterOrEqualsOperator") return TokenType::GreaterOrEqualsOperator;
	if (name == L"LessOperator") return TokenType::LessOperator;
	if (name == L"LessOrEqualsOperator") return TokenType::LessOrEqualsOperator;

	if (name == L"IncrementOperator") return TokenType::IncrementOperator;
	if (name == L"DecrementOperator") return TokenType::DecrementOperator;
	if (name == L"NotOperator") return TokenType::NotOperator;
	if (name == L"AsOperator") return TokenType::AsOperator;

	return TokenType::Unknown;
}
