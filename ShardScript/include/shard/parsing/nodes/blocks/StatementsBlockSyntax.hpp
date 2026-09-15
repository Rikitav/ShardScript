#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxToken.hpp>

#include <shard/parsing/nodes/BodySyntax.hpp>
#include <shard/parsing/nodes/StatementSyntax.hpp>

#include <gmt/Ref.hpp>
#include <gmt/Span.hpp>

namespace shard
{
	class SHARD_API StatementsBlockSyntax final : public BodySyntax
	{
		gmt::Span<gmt::Ref<StatementSyntax>> m_statements;
		SyntaxToken m_openBracketToken;
		SyntaxToken m_closeBracketToken;

	public:
		StatementsBlockSyntax(gmt::Ref<SyntaxNode> parent);
		virtual ~StatementsBlockSyntax() = default;

		gmt::Span<const gmt::Ref<StatementSyntax>> get_statements() const;
		SyntaxToken get_open_bracket() const;
		SyntaxToken get_close_bracket() const;

		void set_statements(gmt::Span<gmt::Ref<StatementSyntax>> statements);
		void set_open_bracket(const SyntaxToken& token);
		void set_close_bracket(const SyntaxToken& token);

		TextLocation get_location() const override;
		void accept(SyntaxVisitor& visitor) const override;
	};
}
