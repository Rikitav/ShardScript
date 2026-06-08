#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/syntax/nodes/ExpressionSyntax.hpp>
#include <shard/syntax/SyntaxKind.hpp>
#include <shard/syntax/SyntaxToken.hpp>
#include <shard/syntax/SyntaxNode.hpp>

#include <vector>

namespace shard
{
	class SHARD_API SwitchArmSyntax : public SyntaxNode
	{
	public:
		SyntaxToken ArrowToken;
		ExpressionSyntax* Pattern = nullptr;
		ExpressionSyntax* Expression = nullptr;

		inline SwitchArmSyntax(SyntaxNode* const parent)
			: SyntaxNode(SyntaxKind::Unknown, parent) { }

		inline SwitchArmSyntax(const SwitchArmSyntax&) = delete;

		inline virtual ~SwitchArmSyntax()
		{
			if (Pattern != nullptr)
				delete Pattern;
			if (Expression != nullptr)
				delete Expression;
		}
	};

	class SHARD_API SwitchExpressionSyntax : public ExpressionSyntax
	{
	public:
		SyntaxToken SwitchKeywordToken;
		SyntaxToken OpenBraceToken;
		SyntaxToken CloseBraceToken;

		ExpressionSyntax* Expression = nullptr;
		std::vector<SwitchArmSyntax*> Arms;

		inline SwitchExpressionSyntax(SyntaxNode* const parent)
			: ExpressionSyntax(SyntaxKind::SwitchExpression, parent) { }

		inline SwitchExpressionSyntax(const SwitchExpressionSyntax&) = delete;

		inline virtual ~SwitchExpressionSyntax()
		{
			if (Expression != nullptr)
				delete Expression;
			for (auto arm : Arms)
				delete arm;
		}
	};
}
