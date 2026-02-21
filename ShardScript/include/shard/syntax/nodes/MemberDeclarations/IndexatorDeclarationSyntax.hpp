#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/syntax/SyntaxKind.hpp>
#include <shard/syntax/SyntaxToken.hpp>
#include <shard/syntax/SyntaxNode.hpp>

#include <shard/parsing/MemberDeclarationInfo.hpp>

#include <shard/syntax/nodes/MemberDeclarationSyntax.hpp>
#include <shard/syntax/nodes/ExpressionSyntax.hpp>
#include <shard/syntax/nodes/TypeSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/AccessorDeclarationSyntax.hpp>
#include <shard/syntax/nodes/ParametersListSyntax.hpp>

#include <vector>

namespace shard
{
    class SHARD_API IndexatorDeclarationSyntax : public MemberDeclarationSyntax
    {
    public:
        SyntaxToken IndexKeyword;
        SyntaxToken OpenBraceToken;
        SyntaxToken CloseBraceToken;

        TypeSyntax* ReturnType = nullptr;
        ParametersListSyntax* Parameters = nullptr;
        
        AccessorDeclarationSyntax* Getter = nullptr;
        AccessorDeclarationSyntax* Setter = nullptr;

        inline IndexatorDeclarationSyntax(SyntaxNode *const parent)
            : MemberDeclarationSyntax(SyntaxKind::IndexatorDeclaration, parent) { }

        inline IndexatorDeclarationSyntax(shard::MemberDeclarationInfo& info, SyntaxNode *const parent) 
            : MemberDeclarationSyntax(SyntaxKind::IndexatorDeclaration, parent)
        {
            Modifiers = info.Modifiers;
            IdentifierToken = info.Identifier;
            ReturnType = info.ReturnType;
            TypeParameters = info.Generics;
        }

        inline virtual ~IndexatorDeclarationSyntax()
        {
            if (Getter != nullptr)
            {
                delete Getter;
                Getter = nullptr;
            }

            if (Setter != nullptr)
            {
                delete Setter;
                Setter = nullptr;
            }

            if (ReturnType != nullptr)
            {
                delete ReturnType;
                ReturnType = nullptr;
            }

            if (Parameters != nullptr)
            {
                delete Parameters;
                Parameters = nullptr;
            }
        }
    };
}
