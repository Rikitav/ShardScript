#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/syntax/SyntaxKind.hpp>
#include <shard/syntax/SyntaxToken.hpp>
#include <shard/syntax/SyntaxNode.hpp>
#include <shard/syntax/nodes/ExpressionSyntax.hpp>
#include <shard/syntax/SyntaxSymbol.hpp>

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
