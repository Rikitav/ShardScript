#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxToken.hpp>

#include <shard/parsing/nodes/statements/ConditionalClauseBaseSyntax.hpp>

namespace shard
{
	class SHARD_API ElseStatementSyntax final : public ConditionalClauseBaseSyntax
	{
		SyntaxToken m_elseKeywordToken;

	public:
		ElseStatementSyntax(gmt::Ref<SyntaxNode> parent);
		virtual ~ElseStatementSyntax() = default;

		SyntaxToken get_else_keyword() const;
		void set_else_keyword(const SyntaxToken& token);

		TextLocation get_location() const override;
		void accept(SyntaxVisitor& visitor) const override;
	};
}
