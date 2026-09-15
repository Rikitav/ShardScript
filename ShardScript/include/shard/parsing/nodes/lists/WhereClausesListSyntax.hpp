#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxNode.hpp>
#include <shard/parsing/SyntaxToken.hpp>

#include <shard/parsing/nodes/TypeSyntax.hpp>

#include <gmt/Ref.hpp>
#include <gmt/Span.hpp>

namespace shard
{
	class SHARD_API WhereClauseSyntax final : public SyntaxNode
	{
		SyntaxToken m_whereKeywordToken;
		SyntaxToken m_identifierToken;
		SyntaxToken m_colonToken;
		gmt::Span<gmt::Ref<TypeSyntax>> m_constraintTypes;

	public:
		WhereClauseSyntax(gmt::Ref<SyntaxNode> parent);
		virtual ~WhereClauseSyntax() = default;

		SyntaxToken get_where_keyword() const;
		SyntaxToken get_identifier() const;
		SyntaxToken get_colon() const;
		gmt::Span<const gmt::Ref<TypeSyntax>> get_constraint_types() const;

		void set_where_keyword(const SyntaxToken& token);
		void set_identifier(const SyntaxToken& token);
		void set_colon(const SyntaxToken& token);
		void set_constraint_types(gmt::Span<gmt::Ref<TypeSyntax>> constraintTypes);

		TextLocation get_location() const override;
		void accept(SyntaxVisitor& visitor) const override;
	};

	class SHARD_API WhereClausesListSyntax final : public SyntaxNode
	{
		gmt::Span<gmt::Ref<WhereClauseSyntax>> m_clauses;

	public:
		WhereClausesListSyntax(gmt::Ref<SyntaxNode> parent);
		virtual ~WhereClausesListSyntax() = default;

		gmt::Span<const gmt::Ref<WhereClauseSyntax>> get_clauses() const;
		void set_clauses(gmt::Span<gmt::Ref<WhereClauseSyntax>> clauses);

		TextLocation get_location() const override;
		void accept(SyntaxVisitor& visitor) const override;
	};
}
