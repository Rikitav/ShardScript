#include <shard/syntax/symbols/TypeParameterSymbol.hpp>
#include <shard/syntax/symbols/MethodSymbol.hpp>

using namespace shard;

void MethodSymbol::OnSymbolDeclared(SyntaxSymbol* symbol)
{
	switch (symbol->Kind)
	{
		case SyntaxKind::TypeParameter:
		{
			TypeParameterSymbol* typeParam = static_cast<TypeParameterSymbol*>(symbol);
			// TODO: add reg
			//TypeParameters.push_back(typeParam);
			break;
		}
	}
}
