#include <shard/syntax/SyntaxFacts.h>
#include <shard/syntax/TokenType.h>

using namespace shard::syntax;

int GetOperatorPrecendence(TokenType type)
{
	switch (type)
	{
		case TokenType::IncrementOperator:
		case TokenType::DecrementOperator:
			return 9;

		case TokenType::MultOperator:
		case TokenType::DivOperator:
		case TokenType::ModOperator:
		case TokenType::PowOperator:
			return 8;

		case TokenType::AddOperator:
		case TokenType::SubOperator:
			return 7;

		case TokenType::LessOperator:
		case TokenType::LessOrEqualsOperator:
		case TokenType::GreaterOperator:
		case TokenType::GreaterOrEqualsOperator:
			return 6;

		case TokenType::LeftShiftOperator:
		case TokenType::RightShiftOperator:
			return 5;

		case TokenType::EqualsOperator:
		case TokenType::NotEqualsOperator:
			return 4;

		case TokenType::OrOperator:
		case TokenType::AndOperator:
			return 3;

		case TokenType::AddAssignOperator:
		case TokenType::SubAssignOperator:
		case TokenType::MultAssignOperator:
		case TokenType::DivAssignOperator:
		case TokenType::ModAssignOperator:
		case TokenType::PowAssignOperator:
		case TokenType::OrAssignOperator:
		case TokenType::AndAssignOperator:
			return 2;

		case TokenType::AssignOperator:
			return 1;

		default:
			return 0;
	}
}

bool IsBinaryArithmeticOperator(TokenType type)
{
	switch (type)
	{
		case TokenType::AddOperator:
		case TokenType::SubOperator:
		case TokenType::MultOperator:
		case TokenType::DivOperator:
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

bool IsBinaryBooleanOperator(TokenType type)
{
	switch (type)
	{
		case TokenType::LessOperator:
		case TokenType::LessOrEqualsOperator:
		case TokenType::GreaterOperator:
		case TokenType::GreaterOrEqualsOperator:
		case TokenType::EqualsOperator:
		case TokenType::NotEqualsOperator:
			return true;

		default:
			return false;
	}
}

bool IsBinaryOperator(TokenType type)
{
	if (IsBinaryArithmeticOperator(type))
		return true;

	if (IsBinaryBooleanOperator(type))
		return true;

	return false;
}

bool IsUnaryArithmeticOperator(TokenType type)
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

bool IsUnaryBooleanOperator(TokenType type)
{
	switch (type)
	{
		case TokenType::NotOperator:
			return true;

		default:
			return false;
	}
}

bool IsUnaryOperator(TokenType type)
{
	if (IsUnaryArithmeticOperator(type))
		return true;

	if (IsUnaryBooleanOperator(type))
		return true;

	return false;
}

bool IsOperator(TokenType type)
{
	if (type == TokenType::AssignOperator)
		return true;

	if (IsBinaryArithmeticOperator(type))
		return true;

	if (IsBinaryBooleanOperator(type))
		return true;

	if (IsBinaryArithmeticOperator(type))
		return true;

	if (IsBinaryArithmeticOperator(type))
		return true;

	return false;
}

bool IsMemberKeyword(TokenType type)
{
	switch (type)
	{
		case TokenType::ClassKeyword:
		case TokenType::StructKeyword:
		case TokenType::InterfaceKeyword:
			return true;

		default:
			return false;
	}
}

bool IsPredefinedType(TokenType type)
{
	switch (type)
	{
		case TokenType::VoidKeyword:
		case TokenType::IntegerKeyword:
		case TokenType::CharKeyword:
		case TokenType::StringKeyword:
			return true;

		default:
			return false;
	}
}

bool IsType(TokenType type, TokenType peekType)
{
	if (IsPredefinedType(type))
		return true;

	if (type == TokenType::Identifier)
	{
		switch (peekType)
		{
			case TokenType::LessOperator:
			case TokenType::Identifier:
			case TokenType::Question:
				return true;

			default:
				return false;
		}
	}

	return false;
}

bool IsModifier(TokenType type)
{
	switch (type)
	{
		case TokenType::StaticKeyword:
		case TokenType::PublicKeyword:
		case TokenType::PrivateKeyword:
		case TokenType::InternalKeyword:
		case TokenType::ProtectedKeyword:
			return true;

		default:
			return false;
	}
}

bool IsMemberDeclaration(TokenType currentType, TokenType peekType)
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
		if (peekType == TokenType::Identifier)
			return true;
	}

	if (currentType == TokenType::Identifier)
	{
		if (peekType == TokenType::Identifier)
			return true;
	}

	return false;
}

bool IsLoopKeyword(TokenType type)
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

bool IsConditionalKeyword(TokenType type)
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

bool IsFunctionalKeyword(TokenType type)
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

bool IsKeyword(TokenType type)
{
	if (IsFunctionalKeyword(type))
		return true;

	if (IsConditionalKeyword(type))
		return true;

	if (IsLoopKeyword(type))
		return true;

	return false;
}

bool IsKeywordHasExpression(TokenType type)
{
	switch (type)
	{
		case TokenType::ReturnKeyword:
			return true;

		default:
			return false;
	}
}

bool IsMethodInvokationExpression(TokenType current, TokenType peek)
{
	if (current == TokenType::Identifier)
	{
		if (peek == TokenType::OpenCurl || peek == TokenType::Delimeter)
			return true;
	}

	return false;
}

bool IsPunctuation(TokenType type)
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
		case TokenType::Semicolon:
			return true;

		default:
			return false;
	}
}
