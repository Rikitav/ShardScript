#include <shard/syntax/symbols/NamespaceSymbol.h>

using namespace shard::syntax::symbols;

void NamespaceSymbol::OnSymbolDeclared(SyntaxSymbol* symbol)
{
	if (symbol->IsType())
	{
		TypeSymbol* type = static_cast<TypeSymbol*>(symbol);
		Members.push_back(type);
		Node->Types.push_back(type);
	}
}