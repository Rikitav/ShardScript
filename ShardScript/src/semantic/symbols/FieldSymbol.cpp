#include <shard/semantic/symbols/FieldSymbol.hpp>

#include <gmt/Interner.hpp>

using namespace shard;

FieldSymbol::FieldSymbol(std::wstring_view name, gmt::Ref<SyntaxSymbol> parent)
	: FieldSymbol(gmt::intern(name), parent) { }

FieldSymbol::FieldSymbol(gmt::StringReference name, gmt::Ref<SyntaxSymbol> parent)
	: MemberSymbol(name, SyntaxKind::FieldDeclaration, parent) { }

gmt::Ref<StructSymbol> FieldSymbol::get_return_type() const
{
	return m_returnType;
}

void FieldSymbol::set_return_type(gmt::Ref<StructSymbol> return_type)
{
	m_returnType = return_type;
}

bool FieldSymbol::get_is_enum_value() const
{
	return m_isEnumValue;
}

void FieldSymbol::set_is_enum_value(bool is_enum_value)
{
	m_isEnumValue = is_enum_value;
}

std::int64_t FieldSymbol::get_enum_value() const
{
	return m_enumValue;
}

void FieldSymbol::set_enum_value(std::int64_t enum_value)
{
	m_enumValue = enum_value;
}
