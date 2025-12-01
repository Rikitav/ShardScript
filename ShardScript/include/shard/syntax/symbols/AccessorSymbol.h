#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxSymbol.h>

#include <shard/syntax/symbols/MethodSymbol.h>

#include <string>

namespace shard::syntax::symbols
{
    class SHARD_API AccessorSymbol : public SyntaxSymbol
    {
    public:
        MethodSymbol* Method = nullptr;

        inline AccessorSymbol(std::wstring name)
            : SyntaxSymbol(name, SyntaxKind::AccessorDeclaration) { }

        inline AccessorSymbol(const AccessorSymbol& other) = delete;

        inline virtual ~AccessorSymbol()
        {
            if (Method != nullptr)
                delete Method;
        }
    };
}