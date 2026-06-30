#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxToken.hpp>
#include <shard/parsing/SyntaxNode.hpp>

#include <shard/parsing/MemberDeclarationInfo.hpp>

#include <shard/parsing/nodes/MemberDeclarationSyntax.hpp>
#include <shard/parsing/nodes/ExpressionSyntax.hpp>
#include <shard/parsing/nodes/TypeSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/AccessorDeclarationSyntax.hpp>
#include <shard/parsing/nodes/ParametersListSyntax.hpp>

#include <vector>
#include <memory>

namespace shard
{
    class SHARD_API IndexatorDeclarationSyntax : public MemberDeclarationSyntax
    {
    public:
        SyntaxToken IndexKeyword;
        SyntaxToken OpenBraceToken;
        SyntaxToken CloseBraceToken;

        std::unique_ptr<TypeSyntax> ReturnType = nullptr;
        std::unique_ptr<ParametersListSyntax> ParametersList = nullptr;
        
        std::unique_ptr<AccessorDeclarationSyntax> Getter = nullptr;
        std::unique_ptr<AccessorDeclarationSyntax> Setter = nullptr;

        inline IndexatorDeclarationSyntax(SyntaxNode* parent)
            : MemberDeclarationSyntax(SyntaxKind::IndexatorDeclaration, parent) { }

        inline IndexatorDeclarationSyntax(shard::MemberDeclarationInfo& info, SyntaxNode* parent) 
            : MemberDeclarationSyntax(SyntaxKind::IndexatorDeclaration, parent)
        {
            Attributes = std::move(info.Attributes);
            Modifiers = info.Modifiers;
            IdentifierToken = info.Identifier;
            ReturnType = std::move(info.ReturnType);
            TypeParameters = std::move(info.Generics);
        }

        inline virtual ~IndexatorDeclarationSyntax() = default;
    };
}
