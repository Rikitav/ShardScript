#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/syntax/SyntaxKind.hpp>
#include <shard/syntax/SyntaxSymbol.hpp>

#include <shard/syntax/symbols/MethodSymbol.hpp>

#include <string>

namespace shard
{
    class SHARD_API AccessorSymbol : public MethodSymbol
    {
    public:
        inline AccessorSymbol(std::wstring name) : MethodSymbol(name)
        {
            Accesibility = SymbolAccesibility::Public;
        }

        inline AccessorSymbol(std::wstring name, MethodSymbolDelegate delegate) : MethodSymbol(name, SyntaxKind::AccessorDeclaration)
        {
            FunctionPointer = delegate;
            HandleType = MethodHandleType::External;
            Accesibility = SymbolAccesibility::Public;
        }

        inline AccessorSymbol(const AccessorSymbol& other) = delete;

        inline virtual ~AccessorSymbol()
        {

        }
    };
}