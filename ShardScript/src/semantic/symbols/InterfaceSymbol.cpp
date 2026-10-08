#include <shard/semantic/symbols/InterfaceSymbol.hpp>

#include <gmt/Interner.hpp>

using namespace shard;

InterfaceSymbol::InterfaceSymbol(std::wstring_view name, gmt::Ref<SyntaxSymbol> parent)
	: InterfaceSymbol(gmt::intern(name), parent) { }

InterfaceSymbol::InterfaceSymbol(gmt::StringReference name, gmt::Ref<SyntaxSymbol> parent)
	: StructSymbol(name, SyntaxKind::InterfaceDeclaration, parent) { }
