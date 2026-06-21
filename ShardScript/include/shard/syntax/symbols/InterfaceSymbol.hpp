#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/syntax/symbols/TypeSymbol.hpp>

#include <shard/syntax/SyntaxKind.hpp>

#include <string>

namespace shard
{
    class SHARD_API InterfaceSymbol : public TypeSymbol
    {
    public:
        inline InterfaceSymbol(const std::wstring& name) : TypeSymbol(name, SyntaxKind::InterfaceDeclaration)
        {
            Inlining = TypeInlining::ByReference;
        }

        inline InterfaceSymbol(const InterfaceSymbol& other) = delete;

        inline virtual ~InterfaceSymbol() override = default;
    };
}
