#include <shard/semantic/symbols/ConstructorSymbol.hpp>

#include <gmt/Interner.hpp>

using namespace shard;

ConstructorSymbol::ConstructorSymbol(gmt::Ref<SyntaxSymbol> parent)
	: MethodSymbol(gmt::intern(L"new"), SyntaxKind::ConstructorDeclaration, parent) { }