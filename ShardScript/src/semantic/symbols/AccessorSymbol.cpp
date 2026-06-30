#include <shard/semantic/symbols/AccessorSymbol.hpp>

using namespace shard;

void AccessorSymbol::OnSymbolDeclared(SyntaxSymbol* symbol)
{
	switch (symbol->Kind)
	{
		case SyntaxKind::Parameter:
		{
			ParameterSymbol* param = static_cast<ParameterSymbol*>(symbol);
			//param->Parent = this; // Accessors never own their propertys\indexers parameter symbols
			Parameters.push_back(param);
			break;
		}
	}
}
