#include <shard/semantic/symbols/ConstructorSymbol.hpp>

#include <gmt/Interner.hpp>

using namespace shard;

ConstructorSymbol::ConstructorSymbol(gmt::Ref<SyntaxSymbol> parent)
	: ConstructorSymbol(gmt::intern(L"new"), parent) { }
