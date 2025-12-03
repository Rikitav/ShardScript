#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxSymbol.h>

#include <shard/syntax/symbols/MethodSymbol.h>

#include <string>

namespace shard::syntax::symbols
{
    class SHARD_API AccessorSymbol : public MethodSymbol
    {
    public:
        inline AccessorSymbol(std::wstring name) : MethodSymbol(name)
        {
            // hehe
            SyntaxKind* pKind = const_cast<SyntaxKind*>(&Kind);
            *pKind = SyntaxKind::AccessorDeclaration;

            Accesibility = SymbolAccesibility::Public;
        }

        inline AccessorSymbol(std::wstring name, shard::syntax::nodes::StatementsBlockSyntax* body) : MethodSymbol(name, body)
        {
            // hehe
            SyntaxKind* pKind = const_cast<SyntaxKind*>(&Kind);
            *pKind = SyntaxKind::AccessorDeclaration;

            Accesibility = SymbolAccesibility::Public;
        }

        inline AccessorSymbol(std::wstring name, MethodSymbolDelegate delegate) : MethodSymbol(name, delegate)
        {
            // hehe
            SyntaxKind* pKind = const_cast<SyntaxKind*>(&Kind);
            *pKind = SyntaxKind::AccessorDeclaration;

            Accesibility = SymbolAccesibility::Public;
        }

        inline AccessorSymbol(const AccessorSymbol& other) = delete;

        inline virtual ~AccessorSymbol()
        {

        }
    };
}