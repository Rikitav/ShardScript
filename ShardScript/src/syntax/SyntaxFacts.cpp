#include <shard/syntax/SyntaxFacts.hpp>
#include <shard/syntax/TokenType.hpp>
#include <shard/syntax/SyntaxKind.hpp>

using namespace shard;

int GetOperatorPrecendence(shard::TokenType type)
{
	switch (type)
	{
		case TokenType::IncrementOperator:
		case TokenType::DecrementOperator:
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

bool IsBinaryArithmeticOperator(shard::TokenType type)
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

bool IsBinaryBooleanOperator(shard::TokenType type)
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

bool IsBinaryBitOperator(shard::TokenType type)
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

bool IsBinaryOperator(shard::TokenType type)
{
	if (IsBinaryArithmeticOperator(type))
		return true;

	if (IsBinaryBooleanOperator(type))
		return true;

	if (IsBinaryBitOperator(type))
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

bool IsRightUnaryArithmeticOperator(shard::TokenType type)
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

bool IsRightUnaryBooleanOperator(shard::TokenType type)
{
	switch (type)
	{
		case TokenType::NotOperator:
			return true;

		default:
			return false;
	}
}

bool IsLeftUnaryOperator(shard::TokenType type)
{
	if (IsLeftUnaryArithmeticOperator(type))
		return true;

	if (IsLeftUnaryBooleanOperator(type))
		return true;

	return false;
}

bool IsLeftUnaryArithmeticOperator(shard::TokenType type)
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

bool IsLeftUnaryBooleanOperator(shard::TokenType type)
{
	return false;
}

bool IsUnaryOperator(shard::TokenType type)
{
	if (IsLeftUnaryOperator(type))
		return true;

	if (IsRightUnaryOperator(type))
		return true;

	return false;
}

bool IsRightUnaryOperator(shard::TokenType type)
{
	if (IsRightUnaryArithmeticOperator(type))
		return true;

	if (IsRightUnaryBooleanOperator(type))
		return true;

	return false;
}

bool IsOperator(shard::TokenType type)
{
	if (type == TokenType::AssignOperator)
		return true;

	if (IsUnaryOperator(type))
		return true;

	if (IsBinaryOperator(type))
		return true;

	return false;
}

bool IsMemberKeyword(shard::TokenType type)
{
	switch (type)
	{
		case TokenType::IndexerKeyword:
			return true;

		default:
			return false;
	}
}

bool IsPredefinedType(shard::TokenType type)
{
	switch (type)
	{
		case TokenType::VoidKeyword:
		case TokenType::VarKeyword:
		case TokenType::IntegerKeyword:
		case TokenType::DoubleKeyword:
		case TokenType::CharKeyword:
		case TokenType::StringKeyword:
		case TokenType::BooleanKeyword:
			return true;

		default:
			return false;
	}
}

bool IsType(shard::TokenType type, shard::TokenType peekType)
{
	if (IsPredefinedType(type))
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

bool IsModifier(shard::TokenType type)
{
	switch (type)
	{
		case TokenType::PublicKeyword:
		case TokenType::PrivateKeyword:
		case TokenType::ProtectedKeyword:
		case TokenType::InternalKeyword:
		case TokenType::StaticKeyword:
		case TokenType::ExternKeyword:
		case TokenType::ExportKeyword:
			/*
		case TokenType::AbstractKeyword:
		case TokenType::SealedKeyword:
		case TokenType::PartialKeyword:
			*/
			return true;

		default:
			return false;
	}
}

bool IsTypeKeyword(shard::TokenType type)
{
	switch (type)
	{
		case TokenType::ClassKeyword:
		case TokenType::StructKeyword:
		case TokenType::InterfaceKeyword:
		case TokenType::DelegateKeyword:
			return true;

		default:
			return false;
	}
}

bool IsMemberDeclaration(shard::TokenType currentType, shard::TokenType peekType)
{
	if (IsModifier(currentType))
		return true;

	if (IsMemberKeyword(currentType))
		return true;

	if (IsTypeKeyword(currentType))
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

bool IsLoopKeyword(shard::TokenType type)
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

bool IsConditionalKeyword(shard::TokenType type)
{
	switch (type)
	{
		case TokenType::IfKeyword:
		case TokenType::UnlessKeyword:
		case TokenType::ElseKeyword:
			return true;

		default:
			return false;
	}
}

bool IsFunctionalKeyword(shard::TokenType type)
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

bool IsLinkedExpressionNode(shard::SyntaxKind kind)
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

bool IsKeyword(shard::TokenType type)
{
	if (IsFunctionalKeyword(type))
		return true;

	if (IsConditionalKeyword(type))
		return true;

	if (IsLoopKeyword(type))
		return true;

	return false;
}

/*
bool IsKeywordHasExpression(shard::TokenType type)
{
	switch (type)
	{
		case TokenType::ReturnKeyword:
			return true;

		default:
			return false;
	}
}

bool IsMethodInvokationExpression(shard::TokenType current, TokenType peek)
{
	if (current == TokenType::Identifier)
	{
		if (peek == TokenType::OpenCurl || peek == TokenType::Delimeter)
			return true;
	}

	return false;
}
*/

bool IsPunctuation(shard::TokenType type)
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

bool IsReservedIdentifier(shard::TokenType type)
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

std::wstring GetOperatorMethodName(shard::TokenType type)
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

		default: return L"";
	}
}

bool IsOverloadableOperator(shard::TokenType type)
{
	return !GetOperatorMethodName(type).empty();
}

shard::TokenType GetTokenTypeFromOperatorName(const std::wstring& name)
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

	return TokenType::Unknown;
}
