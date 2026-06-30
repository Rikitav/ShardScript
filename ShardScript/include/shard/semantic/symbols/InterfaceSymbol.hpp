#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/semantic/symbols/TypeSymbol.hpp>

#include <shard/parsing/SyntaxKind.hpp>

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
