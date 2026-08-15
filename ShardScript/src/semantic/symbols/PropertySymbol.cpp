#include <shard/semantic/symbols/PropertySymbol.hpp>

using namespace shard;

void PropertySymbol::OnSymbolDeclared(SyntaxSymbol* symbol)
{
	if (symbol->Kind == SyntaxKind::AccessorDeclaration)
	{
		symbol->Parent = this;
		symbol->FullName = FullName + L"." + symbol->Name;
	}
}