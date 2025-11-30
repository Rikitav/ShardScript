#include <shard/syntax/SymbolFactory.h>
#include <shard/syntax/SyntaxHelpers.h>

using namespace shard::syntax;
using namespace shard::syntax::nodes;
using namespace shard::syntax::symbols;

MethodSymbol* SymbolFactory::Method(MethodDeclarationSyntax* node)
{
    std::wstring methodName = node->IdentifierToken.Word;
    MethodSymbol* symbol = new MethodSymbol(methodName, node->Body);
    SetAccesibility(symbol, node->Modifiers);

    for (ParameterSyntax* parameter : node->Params->Parameters)
    {
        ParameterSymbol* paramSymbol = new ParameterSymbol(parameter->Identifier.Word);
        symbol->Parameters.push_back(paramSymbol);
    }

    return symbol;
}
