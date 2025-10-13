#pragma once
#include <shard/syntax/TokenType.h>

using namespace shard::syntax;

static int GetOperatorPrecendence(TokenType type)
{
	switch (type)
	{
		default:
			return 0;

		case TokenType::AssignOperator:
			return 1;

		case TokenType::EqualsOperator:
		case TokenType::NotEqualsOperator:
		case TokenType::LessOperator:
		case TokenType::LessOrEqualsOperator:
		case TokenType::GreaterOperator:
		case TokenType::GreaterOrEqualsOperator:
			return 2;

		case TokenType::AddOperator:
		case TokenType::SubOperator:
			return 3;

		case TokenType::MultOperator:
		case TokenType::DivOperator:
		case TokenType::ModOperator:
		case TokenType::PowOperator:
			return 4;
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

bool IsType(TokenType type)
{
	switch (type)
	{
		case TokenType::VoidKeyword:
		case TokenType::IntegerKeyword:
		case TokenType::StringKeyword:
		case TokenType::Identifier:
			return true;

		default:
			return false;
	}
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

bool IsMemberDeclaration(TokenType type)
{
	if (IsMemberKeyword(type))
		return true;

	if (IsModifier(type))
		return true;

	if (IsType(type))
		return true;

	if (type == TokenType::Identifier)
		return true;

	return false;
}

bool IsKeyword(TokenType type)
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
