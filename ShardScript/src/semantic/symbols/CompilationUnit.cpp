#include <shard/semantic/symbols/CompilationUnit.hpp>

using namespace shard;

void CompilationUnit::OnSymbolDeclared(SyntaxSymbol* symbol)
{
	if (symbol->IsType() || symbol->IsMember())
	{
		symbol->Parent = this;
		symbol->FullName = symbol->Name;
	}
}