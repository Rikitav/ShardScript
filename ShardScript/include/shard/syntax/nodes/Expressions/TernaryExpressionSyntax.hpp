#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/syntax/nodes/ExpressionSyntax.hpp>
#include <shard/syntax/SyntaxKind.hpp>
#include <shard/syntax/SyntaxToken.hpp>
#include <shard/syntax/SyntaxNode.hpp>

#include <memory>

namespace shard
{
	class SHARD_API TernaryExpressionSyntax : public ExpressionSyntax
	{
	public:
		SyntaxToken QuestionToken;
		SyntaxToken ColonToken;
		std::unique_ptr<ExpressionSyntax> Condition = nullptr;
		std::unique_ptr<ExpressionSyntax> Left = nullptr;
		std::unique_ptr<ExpressionSyntax> Right = nullptr;

		inline TernaryExpressionSyntax(SyntaxNode *const parent)
			: ExpressionSyntax(SyntaxKind::TernaryExpression, parent) { }

		inline TernaryExpressionSyntax(const TernaryExpressionSyntax&) = delete;

		inline virtual ~TernaryExpressionSyntax() = default;
	};
}
