#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxToken.hpp>

#include <shard/parsing/nodes/ExpressionSyntax.hpp>
#include <shard/parsing/nodes/TypeSyntax.hpp>
#include <shard/parsing/nodes/lists/ArgumentsListSyntax.hpp>

#include <gmt/Ref.hpp>

namespace shard
{
	class SHARD_API ObjectExpressionSyntax final : public ExpressionSyntax
	{
		SyntaxToken m_newToken;
		TypeSyntax* m_type = nullptr;
		ArgumentsListSyntax* m_arguments = nullptr;
		ExpressionSyntax* m_arraySize = nullptr;

	public:
		ObjectExpressionSyntax(SyntaxNode* parent);
		virtual ~ObjectExpressionSyntax() = default;

		SyntaxToken get_new_token() const;
		const TypeSyntax* get_type() const;
		const ArgumentsListSyntax* get_arguments() const;
		const ExpressionSyntax* get_array_size() const;

		bool is_array_creation() const;

		void set_new_token(const SyntaxToken& token);
		void set_type(TypeSyntax* type);
		void set_arguments(ArgumentsListSyntax* arguments);
		void set_array_size(ExpressionSyntax* arraySize);

		TextLocation get_location() const override;
		void accept(SyntaxVisitor& visitor) const override;
	};
}