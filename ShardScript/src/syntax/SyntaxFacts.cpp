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

		case TokenType::LessOperator:
		case TokenType::LessOrEqualsOperator:
		case TokenType::GreaterOperator:
		case TokenType::GreaterOrEqualsOperator:
			return 6;

		case TokenType::EqualsOperator:
		case TokenType::NotEqualsOperator:
			return 5;

		case TokenType::AndOperator:
			return 4;

		case TokenType::OrOperator:
			return 3;

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

	/*
	if (IsType(currentType, peekType))
	{
		if (peekType == TokenType::Identifier)
			return true;
	}
	*/

	if (IsMemberKeyword(currentType))
	{
		return true;
	}

	if (IsTypeKeyword(currentType))
	{
		return true;
	}

	if (currentType == TokenType::Identifier)
	{
		if (peekType == TokenType::Identifier)
			return true;
	}

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
