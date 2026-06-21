#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/syntax/nodes/ExpressionSyntax.hpp>
#include <shard/syntax/SyntaxKind.hpp>
#include <shard/syntax/SyntaxToken.hpp>
#include <shard/syntax/SyntaxNode.hpp>

#include <memory>

namespace shard
{
	class SHARD_API IfExpressionSyntax : public ExpressionSyntax
	{
	public:
		SyntaxToken IfKeywordToken;
		SyntaxToken OpenBraceToken;
		SyntaxToken CloseBraceToken;
		SyntaxToken ElseKeywordToken;

		std::unique_ptr<ExpressionSyntax> Condition = nullptr;
		std::unique_ptr<ExpressionSyntax> ThenExpression = nullptr;
		std::unique_ptr<ExpressionSyntax> ElseExpression = nullptr;

		inline IfExpressionSyntax(SyntaxNode* parent)
			: ExpressionSyntax(SyntaxKind::IfExpression, parent) { }

		inline IfExpressionSyntax(const IfExpressionSyntax&) = delete;

		inline virtual ~IfExpressionSyntax() = default;
	};
}
