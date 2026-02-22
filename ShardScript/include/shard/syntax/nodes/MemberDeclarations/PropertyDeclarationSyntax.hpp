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

namespace shard
{
	class SHARD_API PropertyDeclarationSyntax : public MemberDeclarationSyntax
	{
	public:
		SyntaxToken ReturnTypeToken;
		SyntaxToken OpenBraceToken;
		SyntaxToken CloseBraceToken;
		SyntaxToken SemicolonToken;

		TypeSyntax* ReturnType = nullptr;
		
		AccessorDeclarationSyntax* Getter = nullptr;
		AccessorDeclarationSyntax* Setter = nullptr;
		
		ExpressionSyntax* InitializerExpression = nullptr;

		inline PropertyDeclarationSyntax(SyntaxNode *const parent)
			: MemberDeclarationSyntax(SyntaxKind::PropertyDeclaration, parent) { }

		inline PropertyDeclarationSyntax(shard::MemberDeclarationInfo& info, SyntaxNode *const parent) 
			: MemberDeclarationSyntax(SyntaxKind::PropertyDeclaration, parent)
		{
			Modifiers = info.Modifiers;
			IdentifierToken = info.Identifier;
			ReturnType = info.ReturnType;
			TypeParameters = info.Generics;
		}

		inline PropertyDeclarationSyntax(const PropertyDeclarationSyntax& other) = delete;

		inline virtual ~PropertyDeclarationSyntax()
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

			if (InitializerExpression != nullptr)
			{
				delete InitializerExpression;
				InitializerExpression = nullptr;
			}

			if (ReturnType != nullptr)
			{
				delete ReturnType;
				ReturnType = nullptr;
			}
		}
	};
}

