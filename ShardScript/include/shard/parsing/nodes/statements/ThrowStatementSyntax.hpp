#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxToken.hpp>

#include <shard/parsing/nodes/StatementSyntax.hpp>
#include <shard/parsing/nodes/ExpressionSyntax.hpp>

#include <gmt/Ref.hpp>

namespace shard
{
	class SHARD_API ThrowStatementSyntax final : public StatementSyntax
	{
		SyntaxToken m_throwKeywordToken;
		ExpressionSyntax* m_expression = nullptr;
		SyntaxToken m_semicolonToken;

	public:
		ThrowStatementSyntax(SyntaxNode* parent);
		virtual ~ThrowStatementSyntax() = default;

		SyntaxToken get_throw_keyword() const;
		const ExpressionSyntax* get_expression() const;
		SyntaxToken get_semicolon() const;

		void set_throw_keyword(const SyntaxToken& token);
		void set_expression(ExpressionSyntax* expression);
		void set_semicolon(const SyntaxToken& token);

		TextLocation get_location() const override;
		void accept(SyntaxVisitor& visitor) const override;
	};
}