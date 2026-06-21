#include <shard/syntax/SyntaxSymbol.hpp>
#include <shard/parsing/semantic/SemanticScope.hpp>

#include <optional>
#include <stdexcept>
#include <string>

using namespace shard;

std::optional<SyntaxSymbol*> SemanticScope::Lookup(const std::wstring& name)
{
    auto lookup = _symbols.find(name);
    if (lookup != _symbols.end())
        return lookup->second;

    if (Namespace != nullptr)
    {
        for (SyntaxSymbol* member : Namespace->Members)
        {
            if (member != nullptr && member->Name == name)
                return member;
        }
    }

    if (Parent != nullptr)
        return Parent->Lookup(name);

    return nullptr;
}

void SemanticScope::DeclareSymbol(SyntaxSymbol* symbol)
{
    if (symbol == nullptr)
        throw std::runtime_error("tried to declare nullptr symbol");

    auto find = _symbols.find(symbol->Name);
    if (find != _symbols.end() && find->second == symbol)
        throw std::runtime_error("Symbol already defined");

    _symbols[symbol->Name] = symbol;

}
