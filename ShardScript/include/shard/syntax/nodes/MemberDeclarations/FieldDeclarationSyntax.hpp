#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/syntax/SyntaxKind.hpp>
#include <shard/syntax/SyntaxToken.hpp>
#include <shard/syntax/SyntaxNode.hpp>

#include <shard/parsing/MemberDeclarationInfo.hpp>

#include <shard/syntax/nodes/MemberDeclarationSyntax.hpp>
#include <shard/syntax/nodes/ExpressionSyntax.hpp>
#include <shard/syntax/nodes/TypeSyntax.hpp>

#include <vector>

namespace shard
{
	class SHARD_API FieldDeclarationSyntax : public MemberDeclarationSyntax
	{
	public:
		SyntaxToken ReturnTypeToken;
		SyntaxToken SemicolonToken;
		SyntaxToken InitializerAssignToken;

		ExpressionSyntax* InitializerExpression = nullptr;
		TypeSyntax* ReturnType = nullptr;

		inline FieldDeclarationSyntax(SyntaxNode *const parent)
			: MemberDeclarationSyntax(SyntaxKind::FieldDeclaration, parent) { }

		inline FieldDeclarationSyntax(shard::MemberDeclarationInfo& info, SyntaxNode *const parent) : MemberDeclarationSyntax(SyntaxKind::FieldDeclaration, parent)
		{
			Modifiers = info.Modifiers;
			IdentifierToken = info.Identifier;
			ReturnType = info.ReturnType;
			TypeParameters = info.Generics;
		}

		inline FieldDeclarationSyntax(const FieldDeclarationSyntax& other) = delete;

		inline virtual ~FieldDeclarationSyntax()
		{
			if (InitializerExpression != nullptr)
				delete InitializerExpression;

			if (ReturnType != nullptr)
				delete ReturnType;
		}
	};
}
