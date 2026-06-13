#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/syntax/nodes/ExpressionSyntax.hpp>
#include <shard/syntax/SyntaxKind.hpp>
#include <shard/syntax/SyntaxToken.hpp>
#include <shard/syntax/SyntaxNode.hpp>

#include <vector>
#include <memory>

namespace shard
{
	class SHARD_API SwitchArmSyntax : public SyntaxNode
	{
	public:
		SyntaxToken ArrowToken;
		std::unique_ptr<ExpressionSyntax> Pattern = nullptr;
		std::unique_ptr<ExpressionSyntax> Expression = nullptr;

		inline SwitchArmSyntax(SyntaxNode* const parent)
			: SyntaxNode(SyntaxKind::Unknown, parent) { }

		inline SwitchArmSyntax(const SwitchArmSyntax&) = delete;

		inline virtual ~SwitchArmSyntax() = default;
	};

	class SHARD_API SwitchExpressionSyntax : public ExpressionSyntax
	{
	public:
		SyntaxToken SwitchKeywordToken;
		SyntaxToken OpenBraceToken;
		SyntaxToken CloseBraceToken;

		std::unique_ptr<ExpressionSyntax> Expression = nullptr;
		std::vector<std::unique_ptr<SwitchArmSyntax>> Arms;

		inline SwitchExpressionSyntax(SyntaxNode* const parent)
			: ExpressionSyntax(SyntaxKind::SwitchExpression, parent) { }

		inline SwitchExpressionSyntax(const SwitchExpressionSyntax&) = delete;

		inline virtual ~SwitchExpressionSyntax() = default;
	};
}
