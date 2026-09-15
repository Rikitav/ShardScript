#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxToken.hpp>

#include <shard/parsing/nodes/ExpressionSyntax.hpp>

namespace shard
{
	class SHARD_API LiteralExpressionSyntax final : public ExpressionSyntax
	{
		SyntaxToken m_literalToken;

	public:
		LiteralExpressionSyntax(gmt::Ref<SyntaxNode> parent);
		virtual ~LiteralExpressionSyntax() = default;

		SyntaxToken get_literal() const;
		void set_literal(const SyntaxToken& token);

		TextLocation get_location() const override;
		void accept(SyntaxVisitor& visitor) const override;
	};
}
