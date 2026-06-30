#include <shard/semantic/symbols/IndexatorSymbol.hpp>
#include <shard/semantic/symbols/TypeParameterSymbol.hpp>

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

	if (Getter != nullptr)
		Getter->OnSymbolDeclared(symbol);

	if (Setter != nullptr)
		Setter->OnSymbolDeclared(symbol);
}
