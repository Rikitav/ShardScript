#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/nodes/statements/ConditionalClauseBaseSyntax.hpp>
#include <shard/parsing/nodes/ExpressionSyntax.hpp>

#include <gmt/Ref.hpp>

namespace shard
{
	class SHARD_API ConditionalClauseSyntax : public ConditionalClauseBaseSyntax
	{
		gmt::Ref<ExpressionSyntax> m_condition;

	public:
		ConditionalClauseSyntax(SyntaxKind kind, gmt::Ref<SyntaxNode> parent);
		virtual ~ConditionalClauseSyntax() = default;

		gmt::Ref<const ExpressionSyntax> get_condition() const;
		void set_condition(gmt::Ref<ExpressionSyntax> condition);
	};
}
