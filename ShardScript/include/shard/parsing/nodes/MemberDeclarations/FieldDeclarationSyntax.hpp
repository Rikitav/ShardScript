#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxToken.hpp>
#include <shard/parsing/SyntaxNode.hpp>

#include <shard/parsing/MemberDeclarationInfo.hpp>

#include <shard/parsing/nodes/MemberDeclarationSyntax.hpp>
#include <shard/parsing/nodes/ExpressionSyntax.hpp>
#include <shard/parsing/nodes/TypeSyntax.hpp>

#include <memory>
#include <vector>

namespace shard
{
	class SHARD_API FieldDeclarationSyntax : public MemberDeclarationSyntax
	{
	public:
		SyntaxToken ReturnTypeToken;
		SyntaxToken SemicolonToken;
		SyntaxToken InitializerAssignToken;

		std::unique_ptr<ExpressionSyntax> InitializerExpression = nullptr;
		std::unique_ptr<TypeSyntax> ReturnType = nullptr;

		inline FieldDeclarationSyntax(SyntaxNode* parent)
			: MemberDeclarationSyntax(SyntaxKind::FieldDeclaration, parent) { }

		inline FieldDeclarationSyntax(shard::MemberDeclarationInfo& info, SyntaxNode* parent) : MemberDeclarationSyntax(SyntaxKind::FieldDeclaration, parent)
		{
			Attributes = std::move(info.Attributes);
			Modifiers = info.Modifiers;
			IdentifierToken = info.Identifier;
			ReturnType = std::move(info.ReturnType);
			TypeParameters = std::move(info.Generics);
		}

		inline FieldDeclarationSyntax(const FieldDeclarationSyntax& other) = delete;

		inline virtual ~FieldDeclarationSyntax() = default;
	};
}
