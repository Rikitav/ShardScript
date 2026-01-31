#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxToken.h>
#include <shard/syntax/SyntaxNode.h>

#include <shard/parsing/MemberDeclarationInfo.h>

#include <shard/syntax/nodes/MemberDeclarationSyntax.h>
#include <shard/syntax/nodes/ExpressionSyntax.h>
#include <shard/syntax/nodes/TypeSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/AccessorDeclarationSyntax.h>

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

		inline PropertyDeclarationSyntax(const SyntaxNode* parent)
			: MemberDeclarationSyntax(SyntaxKind::PropertyDeclaration, parent) { }

		inline PropertyDeclarationSyntax(shard::MemberDeclarationInfo& info, const SyntaxNode* parent) 
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

