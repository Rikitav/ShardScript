#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxToken.hpp>

#include <shard/parsing/nodes/ExpressionSyntax.hpp>

#include <gmt/Ref.hpp>

namespace shard
{
	class SHARD_API MemberAccessExpressionSyntax final : public ExpressionSyntax
	{
		ExpressionSyntax* m_previous = nullptr;
		SyntaxToken m_delimeterToken;
		SyntaxToken m_identifierToken;

	public:
		MemberAccessExpressionSyntax(SyntaxNode* parent);
		virtual ~MemberAccessExpressionSyntax() = default;

		const ExpressionSyntax* get_previous() const;
		SyntaxToken get_delimeter_token() const;
		SyntaxToken get_identifier() const;

		void set_previous(ExpressionSyntax* previous);
		void set_delimeter_token(const SyntaxToken& token);
		void set_identifier(const SyntaxToken& token);

		TextLocation get_location() const override;
		void accept(SyntaxVisitor& visitor) const override;
	};
}