#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/nodes/statements/ConditionalClauseBaseSyntax.hpp>
#include <shard/parsing/nodes/ExpressionSyntax.hpp>

#include <gmt/Ref.hpp>

namespace shard
{
	class SHARD_API ConditionalClauseSyntax : public ConditionalClauseBaseSyntax
	{
		ExpressionSyntax* m_condition = nullptr;

	public:
		ConditionalClauseSyntax(SyntaxKind kind, SyntaxNode* parent);
		virtual ~ConditionalClauseSyntax() = default;

		const ExpressionSyntax* get_condition() const;
		void set_condition(ExpressionSyntax* condition);
	};
}