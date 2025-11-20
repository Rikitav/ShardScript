#include <shard/syntax/SyntaxSymbol.h>
#include <shard/parsing/semantic/SemanticScope.h>
#include <stdexcept>
#include <string>

using namespace shard::syntax;
using namespace shard::parsing::semantic;

SyntaxSymbol* SemanticScope::Lookup(std::wstring& name)
{
    auto lookup = _symbols.find(name);
    if (lookup != _symbols.end())
        return lookup->second;

    SemanticScope* parent = (SemanticScope*)Parent;
    if (parent != nullptr)
        return parent->Lookup(name);

    return nullptr;
}

void SemanticScope::DeclareSymbol(SyntaxSymbol* symbol)
{
    if (_symbols.find(symbol->Name) != _symbols.end())
        throw std::runtime_error("Symbol already defined");

    _symbols[symbol->Name] = symbol;
}

void SemanticScope::RemoveSymbol(SyntaxSymbol* symbol)
{
    _symbols.erase(symbol->Name);
}
