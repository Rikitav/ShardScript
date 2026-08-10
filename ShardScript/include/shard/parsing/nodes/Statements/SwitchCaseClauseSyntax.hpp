#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxToken.hpp>
#include <shard/parsing/SyntaxNode.hpp>

#include <shard/parsing/nodes/StatementsBlockSyntax.hpp>
#include <shard/parsing/nodes/ExpressionSyntax.hpp>

#include <memory>

namespace shard
{
	class OperatorSymbol;

	class SHARD_API SwitchCaseClauseSyntax : public SyntaxNode
	{
	public:
		SyntaxToken KeywordToken;
		SyntaxToken ColonToken;

		std::unique_ptr<ExpressionSyntax> Pattern = nullptr;
		std::unique_ptr<StatementsBlockSyntax> Body = nullptr;
		OperatorSymbol* EqualityOperator = nullptr;

		inline SwitchCaseClauseSyntax(SyntaxNode* parent)
			: SyntaxNode(SyntaxKind::SwitchCaseClause, parent) { }

		inline SwitchCaseClauseSyntax(const SwitchCaseClauseSyntax&) = delete;

		inline virtual ~SwitchCaseClauseSyntax() = default;
	};
}
