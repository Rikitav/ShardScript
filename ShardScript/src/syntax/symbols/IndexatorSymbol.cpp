#include <shard/syntax/symbols/IndexatorSymbol.hpp>
#include <shard/syntax/symbols/TypeParameterSymbol.hpp>

using namespace shard;

void IndexatorSymbol::OnSymbolDeclared(SyntaxSymbol* symbol)
{
	switch (symbol->Kind)
	{
		case SyntaxKind::Parameter:
		{
			ParameterSymbol* param = static_cast<ParameterSymbol*>(symbol);
			param->Parent = this;
			Parameters.push_back(param);
			break;
		}
	}

	Getter->OnSymbolDeclared(symbol);
	Setter->OnSymbolDeclared(symbol);
}
