#pragma once
#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxToken.h>
#include <shard/syntax/SyntaxNode.h>

#include <shard/parsing/lexical/MemberDeclarationInfo.h>

#include <shard/syntax/nodes/MemberDeclarationSyntax.h>
#include <shard/syntax/nodes/ExpressionSyntax.h>
#include <shard/syntax/nodes/TypeSyntax.h>

#include <vector>

namespace shard::syntax::nodes
{
	class FieldDeclarationSyntax : public MemberDeclarationSyntax
	{
	public:
		SyntaxToken ReturnTypeToken;
		SyntaxToken SemicolonToken;
		SyntaxToken InitializerAssignToken;

		ExpressionSyntax* InitializerExpression = nullptr;
		TypeSyntax* ReturnType = nullptr;

		inline FieldDeclarationSyntax(const SyntaxNode* parent)
			: MemberDeclarationSyntax(SyntaxKind::FieldDeclaration, parent) { }

		inline FieldDeclarationSyntax(shard::parsing::lexical::MemberDeclarationInfo& info, const SyntaxNode* parent) : MemberDeclarationSyntax(SyntaxKind::FieldDeclaration, parent)
		{
			Modifiers = info.Modifiers;
			IdentifierToken = info.Identifier;
			ReturnType = info.ReturnType;
		}

		inline FieldDeclarationSyntax(const FieldDeclarationSyntax& other) : MemberDeclarationSyntax(other),
			ReturnTypeToken(other.ReturnTypeToken), SemicolonToken(other.SemicolonToken), InitializerAssignToken(other.InitializerAssignToken),
			InitializerExpression(other.InitializerExpression), ReturnType(other.ReturnType) { }

		inline virtual ~FieldDeclarationSyntax()
		{
			if (InitializerExpression != nullptr)
			{
				InitializerExpression->~ExpressionSyntax();
				delete InitializerExpression;
			}

			if (ReturnType != nullptr)
			{
				ReturnType->~TypeSyntax();
				delete ReturnType;
			}
		}
	};
}
