#include <shard/semantic/symbols/PropertySymbol.hpp>

#include <gmt/Interner.hpp>

using namespace shard;

PropertySymbol::PropertySymbol(std::wstring_view name, const SyntaxKind kind, SyntaxSymbol* parent)
	: PropertySymbol(gmt::intern(name), kind, parent) { }

PropertySymbol::PropertySymbol(gmt::StringReference name, const SyntaxKind kind, SyntaxSymbol* parent)
	: MemberSymbol(name, kind, parent) { }

FieldSymbol* PropertySymbol::get_backing_field() const
{
	return m_backingField;
}

void PropertySymbol::set_backing_field(FieldSymbol* backingField)
{
	m_backingField = backingField;
}

StructSymbol* PropertySymbol::get_return_type() const
{
	return m_returnType;
}

void PropertySymbol::set_return_type(StructSymbol* returnType)
{
	m_returnType = returnType;
}

AccessorSymbol* PropertySymbol::get_getter() const
{
	return m_getter;
}

void PropertySymbol::set_getter(AccessorSymbol* getter)
{
	m_getter = getter;
}

AccessorSymbol* PropertySymbol::get_setter() const
{
	return m_setter;
}

void PropertySymbol::set_setter(AccessorSymbol* setter)
{
	m_setter = setter;
}