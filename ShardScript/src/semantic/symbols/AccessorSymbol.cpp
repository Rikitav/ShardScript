#include <shard/semantic/symbols/AccessorSymbol.hpp>

#include <gmt/Interner.hpp>

using namespace shard;

AccessorSymbol::AccessorSymbol(std::wstring_view name, SyntaxSymbol* parent)
	: AccessorSymbol(gmt::intern(name), parent) { }

AccessorSymbol::AccessorSymbol(gmt::StringReference name, SyntaxSymbol* parent)
	: MethodSymbol(name, SyntaxKind::AccessorDeclaration, parent)
{
	set_accesibility(ACS_PUBLIC);
}