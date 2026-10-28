#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxNode.hpp>
#include <shard/parsing/SyntaxToken.hpp>

#include <shard/parsing/nodes/TypeSyntax.hpp>

#include <gmt/Ref.hpp>
#include <span>

namespace shard
{
	class SHARD_API WhereClauseSyntax final : public SyntaxNode
	{
		SyntaxToken m_whereKeywordToken;
		SyntaxToken m_identifierToken;
		SyntaxToken m_colonToken;
		std::span<TypeSyntax*> m_constraintTypes;

	public:
		WhereClauseSyntax(SyntaxNode* parent);
		virtual ~WhereClauseSyntax() = default;

		SyntaxToken get_where_keyword() const;
		SyntaxToken get_identifier() const;
		SyntaxToken get_colon() const;
		std::span<TypeSyntax* const> get_constraint_types() const;

		void set_where_keyword(const SyntaxToken& token);
		void set_identifier(const SyntaxToken& token);
		void set_colon(const SyntaxToken& token);
		void set_constraint_types(std::span<TypeSyntax*> constraintTypes);

		TextLocation get_location() const override;
		void accept(SyntaxVisitor& visitor) const override;
	};

	class SHARD_API WhereClausesListSyntax final : public SyntaxNode
	{
		std::span<WhereClauseSyntax*> m_clauses;

	public:
		WhereClausesListSyntax(SyntaxNode* parent);
		virtual ~WhereClausesListSyntax() = default;

		std::span<WhereClauseSyntax* const> get_clauses() const;
		void set_clauses(std::span<WhereClauseSyntax*> clauses);

		TextLocation get_location() const override;
		void accept(SyntaxVisitor& visitor) const override;
	};
}