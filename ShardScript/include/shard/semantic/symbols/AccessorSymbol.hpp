#pragma once
#include <string>

#include <shard/ShardScriptAPI.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/semantic/SyntaxSymbol.hpp>
#include <shard/semantic/symbols/MethodSymbol.hpp>

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
        inline virtual ~AccessorSymbol() = default;

        void OnSymbolDeclared(SyntaxSymbol* symbol) override;
    };
}