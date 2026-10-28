#include <shard/semantic/symbols/FieldSymbol.hpp>

#include <gmt/Interner.hpp>

using namespace shard;

FieldSymbol::FieldSymbol(std::wstring_view name, SyntaxSymbol* parent)
	: FieldSymbol(gmt::intern(name), parent) { }

FieldSymbol::FieldSymbol(gmt::StringReference name, SyntaxSymbol* parent)
	: MemberSymbol(name, SyntaxKind::FieldDeclaration, parent) { }

StructSymbol* FieldSymbol::get_return_type() const
{
	return m_returnType;
}

void FieldSymbol::set_return_type(StructSymbol* returnType)
{
	m_returnType = returnType;
}

bool FieldSymbol::get_is_enum_value() const
{
	return m_isEnumValue;
}

void FieldSymbol::set_is_enum_value(bool isEnumValue)
{
	m_isEnumValue = isEnumValue;
}

std::int64_t FieldSymbol::get_enum_value() const
{
	return m_enumValue;
}

void FieldSymbol::set_enum_value(std::int64_t enumValue)
{
	m_enumValue = enumValue;
}