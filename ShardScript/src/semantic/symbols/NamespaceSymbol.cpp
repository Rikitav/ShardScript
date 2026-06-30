#include <shard/semantic/symbols/NamespaceSymbol.hpp>

using namespace shard;

void NamespaceSymbol::OnSymbolDeclared(SyntaxSymbol* symbol)
{
	if (symbol->IsType() || symbol->IsMember())
	{
		symbol->Parent = this;
		symbol->FullName = FullName + L"." + symbol->Name;

		Members.push_back(symbol);
		Node->Members.push_back(symbol);
	}
}