#include <shard/semantic/symbols/ConstructorSymbol.hpp>

#include <gmt/Interner.hpp>

using namespace shard;

ConstructorSymbol::ConstructorSymbol(SyntaxSymbol* parent)
	: MethodSymbol(gmt::intern(L"new"), SyntaxKind::ConstructorDeclaration, parent) { }