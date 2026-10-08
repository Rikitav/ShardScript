#include <shard/semantic/symbols/ConstructorSymbol.hpp>

#include <gmt/Interner.hpp>

using namespace shard;

ConstructorSymbol::ConstructorSymbol(std::wstring_view name, gmt::Ref<SyntaxSymbol> parent)
	: ConstructorSymbol(gmt::intern(name), parent) { }

ConstructorSymbol::ConstructorSymbol(gmt::StringReference name, gmt::Ref<SyntaxSymbol> parent)
	: MethodSymbol(name, SyntaxKind::ConstructorDeclaration, parent) { }
