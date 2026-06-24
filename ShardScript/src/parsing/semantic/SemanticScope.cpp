#include <shard/syntax/SyntaxSymbol.hpp>
#include <shard/parsing/semantic/SemanticScope.hpp>

#include <optional>
#include <stdexcept>
#include <string>

using namespace shard;

static std::string WStringToString(const std::wstring& value)
{
    std::string result;
    result.reserve(value.size());
    for (wchar_t c : value)
        result.push_back(static_cast<char>(c));
    return result;
}

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
        throw std::runtime_error("Symbol already defined: " + WStringToString(symbol->Name));

    _symbols[symbol->Name] = symbol;

}
