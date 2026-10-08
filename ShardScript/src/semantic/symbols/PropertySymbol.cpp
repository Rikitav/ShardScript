#include <shard/semantic/symbols/PropertySymbol.hpp>

#include <gmt/Interner.hpp>

using namespace shard;

PropertySymbol::PropertySymbol(std::wstring_view name, const SyntaxKind kind, gmt::Ref<SyntaxSymbol> parent)
	: PropertySymbol(gmt::intern(name), kind, parent) { }

PropertySymbol::PropertySymbol(gmt::StringReference name, const SyntaxKind kind, gmt::Ref<SyntaxSymbol> parent)
	: MemberSymbol(name, kind, parent) { }

gmt::Ref<FieldSymbol> PropertySymbol::get_backing_field() const
{
	return m_backingField;
}

void PropertySymbol::set_backing_field(gmt::Ref<FieldSymbol> backing_field)
{
	m_backingField = backing_field;
}

gmt::Ref<StructSymbol> PropertySymbol::get_return_type() const
{
	return m_returnType;
}

void PropertySymbol::set_return_type(gmt::Ref<StructSymbol> return_type)
{
	m_returnType = return_type;
}

gmt::Ref<AccessorSymbol> PropertySymbol::get_getter() const
{
	return m_getter;
}

void PropertySymbol::set_getter(gmt::Ref<AccessorSymbol> getter)
{
	m_getter = getter;
}

gmt::Ref<AccessorSymbol> PropertySymbol::get_setter() const
{
	return m_setter;
}

void PropertySymbol::set_setter(gmt::Ref<AccessorSymbol> setter)
{
	m_setter = setter;
}
