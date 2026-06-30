#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/semantic/SyntaxSymbol.hpp>

#include <shard/semantic/symbols/MethodSymbol.hpp>

#include <string>

namespace shard
{
    class SHARD_API ConstructorSymbol : public MethodSymbol
    {
    public:
        inline ConstructorSymbol(std::wstring name)
            : MethodSymbol(name, SyntaxKind::ConstructorDeclaration) { }

        inline ConstructorSymbol(std::wstring name, MethodSymbolDelegate delegate) : MethodSymbol(name, SyntaxKind::ConstructorDeclaration)
        {
            FunctionPointer = delegate;
            HandleType = MethodHandleType::External;
        }

        inline ConstructorSymbol(const ConstructorSymbol& other) = delete;

        inline virtual ~ConstructorSymbol()
        {

        }
    };
}