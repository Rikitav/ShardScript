#include <shard/syntax/symbols/NamespaceSymbol.hpp>

using namespace shard;

void NamespaceSymbol::OnSymbolDeclared(SyntaxSymbol* symbol)
{
	if (symbol->IsType() || symbol->IsMember())
	{
		Members.push_back(symbol);
		Node->Members.push_back(symbol);
	}
}