#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxSymbol.h>

#include <shard/syntax/symbols/MethodSymbol.h>

#include <string>

namespace shard::syntax::symbols
{
    class SHARD_API ConstructorSymbol : public MethodSymbol
    {
    public:
        inline ConstructorSymbol(std::wstring name) : MethodSymbol(name)
        {
            // hehe
            SyntaxKind* pKind = const_cast<SyntaxKind*>(&Kind);
            *pKind = SyntaxKind::ConstructorDeclaration;
        }

        inline ConstructorSymbol(std::wstring name, shard::syntax::nodes::StatementsBlockSyntax* body) : MethodSymbol(name, body)
        {
            // hehe
            SyntaxKind* pKind = const_cast<SyntaxKind*>(&Kind);
            *pKind = SyntaxKind::ConstructorDeclaration;
        }

        inline ConstructorSymbol(std::wstring name, MethodSymbolDelegate delegate) : MethodSymbol(name, delegate)
        {
            // hehe
            SyntaxKind* pKind = const_cast<SyntaxKind*>(&Kind);
            *pKind = SyntaxKind::ConstructorDeclaration;
        }

        inline ConstructorSymbol(const ConstructorSymbol& other) = delete;

        inline virtual ~ConstructorSymbol()
        {

        }
    };
}