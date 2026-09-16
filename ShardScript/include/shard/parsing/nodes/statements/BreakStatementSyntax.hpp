#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxToken.hpp>

#include <shard/parsing/nodes/StatementSyntax.hpp>

namespace shard
{
	class SHARD_API BreakStatementSyntax final : public StatementSyntax
	{
		SyntaxToken m_breakKeywordToken;
		SyntaxToken m_tagToken;
		SyntaxToken m_semicolonToken;

	public:
		BreakStatementSyntax(gmt::Ref<SyntaxNode> parent);
		virtual ~BreakStatementSyntax() = default;

		SyntaxToken get_break_keyword() const;
		SyntaxToken get_tag() const;
		SyntaxToken get_semicolon() const;

		void set_break_keyword(const SyntaxToken& token);
		void set_tag(const SyntaxToken& token);
		void set_semicolon(const SyntaxToken& token);

		TextLocation get_location() const override;
		void accept(SyntaxVisitor& visitor) const override;
	};
}
