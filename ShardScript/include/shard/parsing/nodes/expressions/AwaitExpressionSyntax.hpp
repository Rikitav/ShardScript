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
		ExpressionSyntax* m_expression = nullptr;

	public:
		AwaitExpressionSyntax(SyntaxNode* parent);
		virtual ~AwaitExpressionSyntax() = default;

		SyntaxToken get_await_keyword() const;
		const ExpressionSyntax* get_expression() const;

		void set_await_keyword(const SyntaxToken& token);
		void set_expression(ExpressionSyntax* expression);

		TextLocation get_location() const override;
		void accept(SyntaxVisitor& visitor) const override;
	};
}