#include <shard/semantic/symbols/TypeParameterSymbol.hpp>
#include <shard/semantic/symbols/MethodSymbol.hpp>

using namespace shard;

bool MethodSymbol::IsMethod() const
{
	return true;
}

void MethodSymbol::OnSymbolDeclared(SyntaxSymbol* symbol)
{
	switch (symbol->Kind)
	{
		case SyntaxKind::TypeParameter:
		{
			TypeParameterSymbol* typeParam = static_cast<TypeParameterSymbol*>(symbol);
			typeParam->Parent = this;
			TypeParameters.push_back(typeParam);
			break;
		}

		case SyntaxKind::Parameter:
		{
			ParameterSymbol* param = static_cast<ParameterSymbol*>(symbol);
			param->Parent = this;
			Parameters.push_back(param);
			break;
		}
	}
}

std::uint16_t MethodSymbol::GetEvalStackArgumentsCount() const
{
    std::uint16_t count = static_cast<std::uint16_t>(Parameters.size());
    if (Linking == LINK_INSTANCE)
        count += 1; // implicit 'this'

    return count;
}

std::uint16_t MethodSymbol::GetEvalStackVariablesCount() const
{
    return EvalStackVariablesCount;
}

std::uint16_t MethodSymbol::GetEvalStackLocalsCount() const
{
    return GetEvalStackArgumentsCount() + EvalStackVariablesCount;
}

std::uint16_t MethodSymbol::AddVariableCount()
{
	return EvalStackVariablesCount++;
}
