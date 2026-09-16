#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxToken.hpp>

#include <shard/parsing/nodes/ExpressionSyntax.hpp>

#include <gmt/Ref.hpp>

namespace shard
{
	class SHARD_API AwaitExpressionSyntax final : public ExpressionSyntax
	{
		SyntaxToken m_awaitKeywordToken;
		gmt::Ref<ExpressionSyntax> m_expression;

	public:
		AwaitExpressionSyntax(gmt::Ref<SyntaxNode> parent);
		virtual ~AwaitExpressionSyntax() = default;

		SyntaxToken get_await_keyword() const;
		gmt::Ref<const ExpressionSyntax> get_expression() const;

		void set_await_keyword(const SyntaxToken& token);
		void set_expression(gmt::Ref<ExpressionSyntax> expression);

		TextLocation get_location() const override;
		void accept(SyntaxVisitor& visitor) const override;
	};
}
