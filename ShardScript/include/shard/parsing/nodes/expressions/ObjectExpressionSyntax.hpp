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
		gmt::Ref<TypeSyntax> m_type;
		gmt::Ref<ArgumentsListSyntax> m_arguments;
		gmt::Ref<ExpressionSyntax> m_arraySize;

	public:
		ObjectExpressionSyntax(gmt::Ref<SyntaxNode> parent);
		virtual ~ObjectExpressionSyntax() = default;

		SyntaxToken get_new_token() const;
		gmt::Ref<const TypeSyntax> get_type() const;
		gmt::Ref<const ArgumentsListSyntax> get_arguments() const;
		gmt::Ref<const ExpressionSyntax> get_array_size() const;

		bool is_array_creation() const;

		void set_new_token(const SyntaxToken& token);
		void set_type(gmt::Ref<TypeSyntax> type);
		void set_arguments(gmt::Ref<ArgumentsListSyntax> arguments);
		void set_array_size(gmt::Ref<ExpressionSyntax> arraySize);

		TextLocation get_location() const override;
		void accept(SyntaxVisitor& visitor) const override;
	};
}
