#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxToken.hpp>
#include <shard/parsing/SyntaxNode.hpp>
#include <shard/parsing/nodes/ExpressionSyntax.hpp>
#include <shard/semantic/SyntaxSymbol.hpp>

#include <memory>

namespace shard
{
	class SHARD_API EnumFieldDeclarationSyntax : public SyntaxNode
	{
	public:
		SyntaxToken IdentifierToken;
		SyntaxToken AssignToken;
		std::unique_ptr<ExpressionSyntax> InitializerExpression = nullptr;
		shard::SyntaxSymbol* Symbol = nullptr;

		inline EnumFieldDeclarationSyntax(SyntaxNode* parent)
			: SyntaxNode(SyntaxKind::EnumFieldDeclaration, parent) { }

		inline EnumFieldDeclarationSyntax(const EnumFieldDeclarationSyntax& other) = delete;

		inline virtual ~EnumFieldDeclarationSyntax() = default;
	};
}
