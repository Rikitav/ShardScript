#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxToken.hpp>

#include <shard/parsing/nodes/ExpressionSyntax.hpp>

#include <gmt/Ref.hpp>
#include <span>

namespace shard
{
	class SHARD_API CollectionExpressionSyntax final : public ExpressionSyntax
	{
		std::span<ExpressionSyntax*> m_values;
		SyntaxToken m_openBracketToken;
		SyntaxToken m_closeBracketToken;

	public:
		CollectionExpressionSyntax(SyntaxNode* parent);
		virtual ~CollectionExpressionSyntax() = default;

		std::span<ExpressionSyntax* const> get_values() const;
		SyntaxToken get_open_bracket() const;
		SyntaxToken get_close_bracket() const;

		void set_values(std::span<ExpressionSyntax*> values);
		void set_open_bracket(const SyntaxToken& token);
		void set_close_bracket(const SyntaxToken& token);

		TextLocation get_location() const override;
		void accept(SyntaxVisitor& visitor) const override;
	};
}