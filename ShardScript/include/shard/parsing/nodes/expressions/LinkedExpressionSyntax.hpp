#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxToken.hpp>

#include <shard/parsing/nodes/ExpressionSyntax.hpp>
#include <shard/parsing/nodes/lists/ArgumentsListSyntax.hpp>

#include <gmt/Ref.hpp>

namespace shard
{
	class SHARD_API LinkedExpressionNode : public ExpressionSyntax
	{
	protected:
		gmt::Ref<ExpressionSyntax> m_previous;
		SyntaxToken m_delimeterToken;

	public:
		LinkedExpressionNode(const SyntaxKind kind, gmt::Ref<SyntaxNode> parent);
		virtual ~LinkedExpressionNode() = default;

		gmt::Ref<const ExpressionSyntax> get_previous() const;
		SyntaxToken get_delimeter_token() const;

		void set_previous(gmt::Ref<ExpressionSyntax> previous);
		void set_delimeter_token(const SyntaxToken& token);

		TextLocation get_location() const override;
	};

	class SHARD_API MemberAccessExpressionSyntax final : public LinkedExpressionNode
	{
		SyntaxToken m_identifierToken;

	public:
		MemberAccessExpressionSyntax(gmt::Ref<SyntaxNode> parent);
		virtual ~MemberAccessExpressionSyntax() = default;

		SyntaxToken get_identifier() const;
		void set_identifier(const SyntaxToken& token);

		void accept(SyntaxVisitor& visitor) const override;
	};

	class SHARD_API InvokationExpressionSyntax final : public LinkedExpressionNode
	{
		SyntaxToken m_identifierToken;
		gmt::Ref<ArgumentsListSyntax> m_arguments;

	public:
		InvokationExpressionSyntax(gmt::Ref<SyntaxNode> parent);
		virtual ~InvokationExpressionSyntax() = default;

		SyntaxToken get_identifier() const;
		gmt::Ref<const ArgumentsListSyntax> get_arguments() const;

		void set_identifier(const SyntaxToken& token);
		void set_arguments(gmt::Ref<ArgumentsListSyntax> arguments);

		void accept(SyntaxVisitor& visitor) const override;
	};
}
