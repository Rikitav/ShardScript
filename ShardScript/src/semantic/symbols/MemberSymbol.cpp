#include <shard/semantic/symbols/MemberSymbol.hpp>

using namespace shard;

MemberSymbol::MemberSymbol(std::wstring_view name, const SyntaxKind kind, gmt::Ref<SyntaxSymbol> parent)
	: SyntaxSymbol(name, kind, parent) { }

MemberSymbol::MemberSymbol(gmt::StringReference name, const SyntaxKind kind, gmt::Ref<SyntaxSymbol> parent)
	: SyntaxSymbol(name, kind, parent) { }

SymbolLinking MemberSymbol::get_linking() const
{
	return m_linking;
}

void MemberSymbol::set_linking(SymbolLinking linking)
{
	m_linking = linking;
}

SymbolAccesibility MemberSymbol::get_accesibility() const
{
	return m_accesibility;
}

void MemberSymbol::set_accesibility(SymbolAccesibility accesibility)
{
	m_accesibility = accesibility;
}

bool MemberSymbol::is_member() const
{
	return true;
}
