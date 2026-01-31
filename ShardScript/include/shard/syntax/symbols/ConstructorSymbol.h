#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxSymbol.h>

#include <shard/syntax/symbols/MethodSymbol.h>

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