#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/semantic/SyntaxSymbol.hpp>

namespace shard
{
    class SHARD_API CompilationUnit : public SyntaxSymbol
    {
    public:
        inline CompilationUnit()
            : SyntaxSymbol(L"<>k__CompilationUnit", SyntaxKind::CompilationUnit) {}

        inline CompilationUnit(const CompilationUnit& other) = delete;

        inline virtual ~CompilationUnit() = default;

        void OnSymbolDeclared(SyntaxSymbol* symbol) override;
    };
}
