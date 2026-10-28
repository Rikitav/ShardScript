#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxToken.hpp>

#include <shard/parsing/nodes/ExpressionSyntax.hpp>
#include <shard/parsing/nodes/lists/ArgumentsListSyntax.hpp>

#include <gmt/Ref.hpp>

namespace shard
{
	class SHARD_API IndexatorExpressionSyntax final : public ExpressionSyntax
	{
		ExpressionSyntax* m_previous = nullptr;
		SyntaxToken m_delimeterToken;
		ArgumentsListSyntax* m_arguments = nullptr;

	public:
		IndexatorExpressionSyntax(SyntaxNode* parent);
		virtual ~IndexatorExpressionSyntax() = default;

		const ExpressionSyntax* get_previous() const;
		SyntaxToken get_delimeter_token() const;
		const ArgumentsListSyntax* get_arguments() const;

		void set_previous(ExpressionSyntax* previous);
		void set_delimeter_token(const SyntaxToken& token);
		void set_arguments(ArgumentsListSyntax* arguments);

		TextLocation get_location() const override;
		void accept(SyntaxVisitor& visitor) const override;
	};
}