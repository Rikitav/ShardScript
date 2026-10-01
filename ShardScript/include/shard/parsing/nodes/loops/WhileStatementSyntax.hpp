#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxToken.hpp>

#include <shard/parsing/nodes/StatementSyntax.hpp>
#include <shard/parsing/nodes/ExpressionSyntax.hpp>
#include <shard/parsing/nodes/blocks/StatementsBlockSyntax.hpp>

#include <gmt/Ref.hpp>

namespace shard
{
	class SHARD_API WhileStatementSyntax final : public StatementSyntax
	{
		SyntaxToken m_whileKeywordToken;
		SyntaxToken m_openCurlToken;
		SyntaxToken m_closeCurlToken;
		gmt::Ref<ExpressionSyntax> m_condition;
		gmt::Ref<StatementsBlockSyntax> m_block;

	public:
		WhileStatementSyntax(gmt::Ref<SyntaxNode> parent);
		virtual ~WhileStatementSyntax() = default;

		SyntaxToken get_while_keyword() const;
		SyntaxToken get_open_curl() const;
		SyntaxToken get_close_curl() const;
		gmt::Ref<const ExpressionSyntax> get_condition() const;
		gmt::Ref<const StatementsBlockSyntax> get_block() const;

		void set_while_keyword(const SyntaxToken& token);
		void set_open_curl(const SyntaxToken& token);
		void set_close_curl(const SyntaxToken& token);
		void set_condition(gmt::Ref<ExpressionSyntax> condition);
		void set_block(gmt::Ref<StatementsBlockSyntax> block);

		TextLocation get_location() const override;
		void accept(SyntaxVisitor& visitor) const override;
	};
}
