#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxToken.hpp>

#include <shard/parsing/nodes/statements/ConditionalClauseSyntax.hpp>

namespace shard
{
	class SHARD_API IfStatementSyntax final : public ConditionalClauseSyntax
	{
		SyntaxToken m_ifKeywordToken;

	public:
		IfStatementSyntax(gmt::Ref<SyntaxNode> parent);
		virtual ~IfStatementSyntax() = default;

		SyntaxToken get_if_keyword() const;
		void set_if_keyword(const SyntaxToken& token);

		TextLocation get_location() const override;
		void accept(SyntaxVisitor& visitor) const override;
	};
}
