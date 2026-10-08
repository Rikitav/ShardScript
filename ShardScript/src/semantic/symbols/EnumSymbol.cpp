#include <shard/semantic/symbols/EnumSymbol.hpp>

#include <gmt/Interner.hpp>

using namespace shard;

EnumSymbol::EnumSymbol(std::wstring_view name, gmt::Ref<SyntaxSymbol> parent)
	: EnumSymbol(gmt::intern(name), parent) { }

EnumSymbol::EnumSymbol(gmt::StringReference name, gmt::Ref<SyntaxSymbol> parent)
	: StructSymbol(name, SyntaxKind::EnumDeclaration, parent) { }

bool EnumSymbol::get_is_flags() const
{
	return m_isFlags;
}

void EnumSymbol::set_is_flags(bool is_flags)
{
	m_isFlags = is_flags;
}
