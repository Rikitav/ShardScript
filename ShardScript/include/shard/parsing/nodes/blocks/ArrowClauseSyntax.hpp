#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxToken.hpp>

#include <shard/parsing/nodes/BlockSyntax.hpp>
#include <shard/parsing/nodes/ExpressionSyntax.hpp>

#include <gmt/Ref.hpp>

namespace shard
{
	class SHARD_API ArrowClauseSyntax final : public BlockSyntax
	{
		ExpressionSyntax* m_expression = nullptr;
		SyntaxToken m_arrowToken;

	public:
		ArrowClauseSyntax(SyntaxNode* parent);
		virtual ~ArrowClauseSyntax() = default;

		const ExpressionSyntax* get_expression() const;
		SyntaxToken get_arrow_token() const;

		void set_expression(ExpressionSyntax* expression);
		void set_arrow_token(const SyntaxToken& token);

		TextLocation get_location() const override;
		void accept(SyntaxVisitor& visitor) const override;
	};
}