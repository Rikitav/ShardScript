#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxToken.hpp>

#include <shard/parsing/nodes/StatementSyntax.hpp>
#include <shard/parsing/nodes/statements/VariableStatementSyntax.hpp>

#include <gmt/Ref.hpp>

namespace shard
{
	class SHARD_API DeferVariableSyntax final : public StatementSyntax
	{
		SyntaxToken m_deferKeywordToken;
		gmt::Ref<VariableStatementSyntax> m_variable;

	public:
		DeferVariableSyntax(gmt::Ref<SyntaxNode> parent);
		virtual ~DeferVariableSyntax() = default;

		SyntaxToken get_defer_keyword() const;
		gmt::Ref<const VariableStatementSyntax> get_variable() const;

		void set_defer_keyword(const SyntaxToken& token);
		void set_variable(gmt::Ref<VariableStatementSyntax> variable);

		TextLocation get_location() const override;
		void accept(SyntaxVisitor& visitor) const override;
	};
}
