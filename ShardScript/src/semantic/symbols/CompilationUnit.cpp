#include <shard/semantic/symbols/CompilationUnit.hpp>

#include <gmt/Interner.hpp>
#include <gmt/NullRef.hpp>

using namespace shard;

CompilationUnit::CompilationUnit()
	: SyntaxSymbol(gmt::intern(L"<>k__CompilationUnit"), SyntaxKind::CompilationUnit, gmt::nullref) { }
