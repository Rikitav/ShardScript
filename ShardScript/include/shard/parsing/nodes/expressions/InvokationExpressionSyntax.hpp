#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxToken.hpp>

#include <shard/parsing/nodes/ExpressionSyntax.hpp>
#include <shard/parsing/nodes/lists/ArgumentsListSyntax.hpp>
#include <shard/parsing/nodes/lists/TypeArgumentsListSyntax.hpp>

#include <gmt/Ref.hpp>

namespace shard
{
	class SHARD_API InvokationExpressionSyntax final : public ExpressionSyntax
	{
		ExpressionSyntax* m_previous = nullptr;
		SyntaxToken m_delimeterToken;
		SyntaxToken m_identifierToken;
		TypeArgumentsListSyntax* m_typeArguments = nullptr;
		ArgumentsListSyntax* m_arguments = nullptr;

	public:
		InvokationExpressionSyntax(SyntaxNode* parent);
		virtual ~InvokationExpressionSyntax() = default;

		const ExpressionSyntax* get_previous() const;
		SyntaxToken get_delimeter_token() const;
		SyntaxToken get_identifier() const;
		const TypeArgumentsListSyntax* get_type_arguments() const;
		const ArgumentsListSyntax* get_arguments() const;

		void set_previous(ExpressionSyntax* previous);
		void set_delimeter_token(const SyntaxToken& token);
		void set_identifier(const SyntaxToken& token);
		void set_type_arguments(TypeArgumentsListSyntax* typeArguments);
		void set_arguments(ArgumentsListSyntax* arguments);

		TextLocation get_location() const override;
		void accept(SyntaxVisitor& visitor) const override;
	};
}