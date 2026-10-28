#include <shard/semantic/symbols/LiteralSymbol.hpp>

#include <gmt/Interner.hpp>

using namespace shard;

LiteralSymbol::LiteralSymbol(const TokenType literalType, SyntaxSymbol* parent)
	: SyntaxSymbol(gmt::intern(L"Literal"), SyntaxKind::LiteralExpression, parent), m_literalType(literalType) { }

TokenType LiteralSymbol::get_literal_type() const
{
	return m_literalType;
}

bool LiteralSymbol::get_boolean_value() const
{
	return m_asBoolean;
}

void LiteralSymbol::set_boolean_value(bool value)
{
	m_asBoolean = value;
}

std::int64_t LiteralSymbol::get_integer_value() const
{
	return m_asInteger;
}

void LiteralSymbol::set_integer_value(std::int64_t value)
{
	m_asInteger = value;
}

std::double_t LiteralSymbol::get_double_value() const
{
	return m_asDouble;
}

void LiteralSymbol::set_double_value(std::double_t value)
{
	m_asDouble = value;
}

wchar_t LiteralSymbol::get_char_value() const
{
	return m_asChar;
}

void LiteralSymbol::set_char_value(wchar_t value)
{
	m_asChar = value;
}

gmt::StringReference LiteralSymbol::get_string_value() const
{
	return m_asString;
}

void LiteralSymbol::set_string_value(gmt::StringReference value)
{
	m_asString = value;
}

StructSymbol* LiteralSymbol::get_bound_type() const
{
	return m_boundType;
}

void LiteralSymbol::set_bound_type(StructSymbol* boundType)
{
	m_boundType = boundType;
}