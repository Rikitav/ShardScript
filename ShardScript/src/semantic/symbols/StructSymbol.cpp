#include <shard/semantic/symbols/StructSymbol.hpp>

using namespace shard;

StructSymbol::StructSymbol(std::wstring_view name, const SyntaxKind kind, gmt::Ref<SyntaxSymbol> parent)
	: MemberSymbol(name, kind, parent) { }

bool StructSymbol::get_is_nullable() const
{
	return m_is_nullable;
}

void StructSymbol::set_is_nullable(bool is_nullable)
{
	m_is_nullable = is_nullable;
}

bool StructSymbol::is_type() const
{
	return true;
}
