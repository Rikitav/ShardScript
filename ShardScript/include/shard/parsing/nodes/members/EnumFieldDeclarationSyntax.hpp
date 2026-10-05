#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxToken.hpp>

#include <shard/parsing/SyntaxNode.hpp>
#include <shard/parsing/nodes/ExpressionSyntax.hpp>

#include <gmt/Ref.hpp>

namespace shard
{
	class SHARD_API EnumFieldDeclarationSyntax final : public SyntaxNode
	{
		SyntaxToken m_identifierToken;
		SyntaxToken m_assignToken;
		gmt::Ref<ExpressionSyntax> m_expression;

	public:
		EnumFieldDeclarationSyntax(gmt::Ref<SyntaxNode> parent);
		virtual ~EnumFieldDeclarationSyntax() = default;

		SyntaxToken get_identifier() const;
		SyntaxToken get_assign_token() const;
		gmt::Ref<const ExpressionSyntax> get_expression() const;

		void set_identifier(const SyntaxToken& token);
		void set_assign_token(const SyntaxToken& token);
		void set_expression(gmt::Ref<ExpressionSyntax> expression);

		TextLocation get_location() const override;
		void accept(SyntaxVisitor& visitor) const override;
	};
}
