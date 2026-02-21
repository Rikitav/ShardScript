#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/syntax/SyntaxKind.hpp>
#include <shard/syntax/SyntaxSymbol.hpp>

#include <shard/syntax/symbols/MethodSymbol.hpp>

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