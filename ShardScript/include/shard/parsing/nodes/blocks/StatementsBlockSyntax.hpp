#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxToken.hpp>

#include <shard/parsing/nodes/BlockSyntax.hpp>
#include <shard/parsing/nodes/StatementSyntax.hpp>

#include <gmt/Ref.hpp>
#include <span>

namespace shard
{
	class SHARD_API StatementsBlockSyntax final : public BlockSyntax
	{
		std::span<StatementSyntax*> m_statements;
		SyntaxToken m_openBracketToken;
		SyntaxToken m_closeBracketToken;

	public:
		StatementsBlockSyntax(SyntaxNode* parent);
		virtual ~StatementsBlockSyntax() = default;

		std::span<StatementSyntax* const> get_statements() const;
		SyntaxToken get_open_bracket() const;
		SyntaxToken get_close_bracket() const;

		void set_statements(std::span<StatementSyntax*> statements);
		void set_open_bracket(const SyntaxToken& token);
		void set_close_bracket(const SyntaxToken& token);

		TextLocation get_location() const override;
		void accept(SyntaxVisitor& visitor) const override;
	};
}