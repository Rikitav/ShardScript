#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxToken.hpp>

#include <shard/parsing/nodes/ExpressionSyntax.hpp>
#include <shard/parsing/nodes/TypeSyntax.hpp>

#include <gmt/Ref.hpp>

namespace shard
{
	class SHARD_API DefaultExpressionSyntax final : public ExpressionSyntax
	{
		SyntaxToken m_defaultKeyword;
		TypeSyntax* m_type = nullptr;

	public:
		DefaultExpressionSyntax(SyntaxNode* parent);
		virtual ~DefaultExpressionSyntax() = default;

		SyntaxToken get_default_keyword() const;
		const TypeSyntax* get_type() const;

		void set_default_keyword(const SyntaxToken& token);
		void set_type(TypeSyntax* type);

		TextLocation get_location() const override;
		void accept(SyntaxVisitor& visitor) const override;
	};
}
