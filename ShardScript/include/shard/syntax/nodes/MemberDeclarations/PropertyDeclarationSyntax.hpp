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

#include <vector>
#include <memory>

namespace shard
{
	class SHARD_API PropertyDeclarationSyntax : public MemberDeclarationSyntax
	{
	public:
		SyntaxToken ReturnTypeToken;
		SyntaxToken OpenBraceToken;
		SyntaxToken CloseBraceToken;
		SyntaxToken ArrowToken;
		SyntaxToken SemicolonToken;

		std::unique_ptr<AccessorDeclarationSyntax> Getter = nullptr;
		std::unique_ptr<AccessorDeclarationSyntax> Setter = nullptr;
		std::unique_ptr<ExpressionSyntax> InitializerExpression = nullptr;
		
		std::unique_ptr<TypeSyntax> ReturnType = nullptr;

		inline PropertyDeclarationSyntax(SyntaxNode* parent)
			: MemberDeclarationSyntax(SyntaxKind::PropertyDeclaration, parent) { }

		inline PropertyDeclarationSyntax(shard::MemberDeclarationInfo& info, SyntaxNode* parent) 
			: MemberDeclarationSyntax(SyntaxKind::PropertyDeclaration, parent)
		{
			Attributes = std::move(info.Attributes);
			Modifiers = info.Modifiers;
			IdentifierToken = info.Identifier;
			ReturnType = std::move(info.ReturnType);
			TypeParameters = std::move(info.Generics);
		}

		inline PropertyDeclarationSyntax(const PropertyDeclarationSyntax& other) = delete;

		inline virtual ~PropertyDeclarationSyntax() = default;
	};
}
