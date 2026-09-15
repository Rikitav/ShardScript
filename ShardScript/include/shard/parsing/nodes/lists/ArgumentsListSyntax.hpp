#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxNode.hpp>
#include <shard/parsing/SyntaxToken.hpp>

#include <shard/parsing/nodes/ExpressionSyntax.hpp>

#include <gmt/Ref.hpp>
#include <gmt/Span.hpp>

namespace shard
{
	class SHARD_API ArgumentSyntax final : public SyntaxNode
	{
		gmt::Ref<ExpressionSyntax> m_expression;

	public:
		ArgumentSyntax(gmt::Ref<SyntaxNode> parent);
		virtual ~ArgumentSyntax() = default;

		gmt::Ref<const ExpressionSyntax> get_expression() const;
		void set_expression(gmt::Ref<ExpressionSyntax> expression);

		TextLocation get_location() const override;
		void accept(SyntaxVisitor& visitor) const override;
	};

	class SHARD_API ArgumentsListSyntax final : public SyntaxNode
	{
		gmt::Span<gmt::Ref<ArgumentSyntax>> m_arguments;

		SyntaxToken m_openToken;
		SyntaxToken m_closeToken;

	public:
		ArgumentsListSyntax(gmt::Ref<SyntaxNode> parent);
		virtual ~ArgumentsListSyntax() = default;

		gmt::Span<const gmt::Ref<ArgumentSyntax>> get_arguments() const;
		SyntaxToken get_open_token() const;
		SyntaxToken get_close_token() const;

		void set_arguments(gmt::Span<gmt::Ref<ArgumentSyntax>> arguments);
		void set_open_token(const SyntaxToken& token);
		void set_close_token(const SyntaxToken& token);

		TextLocation get_location() const override;
		void accept(SyntaxVisitor& visitor) const override;
	};
}
