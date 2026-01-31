#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxSymbol.h>

#include <shard/syntax/symbols/MethodSymbol.h>

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