#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxToken.hpp>

#include <shard/parsing/nodes/statements/ConditionalClauseSyntax.hpp>

namespace shard
{
	class SHARD_API UnlessStatementSyntax final : public ConditionalClauseSyntax
	{
		SyntaxToken m_unlessKeywordToken;

	public:
		UnlessStatementSyntax(gmt::Ref<SyntaxNode> parent);
		virtual ~UnlessStatementSyntax() = default;

		SyntaxToken get_unless_keyword() const;
		void set_unless_keyword(const SyntaxToken& token);

		TextLocation get_location() const override;
		void accept(SyntaxVisitor& visitor) const override;
	};
}
