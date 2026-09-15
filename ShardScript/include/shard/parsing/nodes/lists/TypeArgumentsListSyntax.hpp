#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxNode.hpp>
#include <shard/parsing/SyntaxToken.hpp>

#include <shard/parsing/nodes/TypeSyntax.hpp>

#include <gmt/Ref.hpp>
#include <gmt/Span.hpp>

namespace shard
{
	class SHARD_API TypeArgumentsListSyntax final : public SyntaxNode
	{
		gmt::Span<gmt::Ref<TypeSyntax>> m_types;

		SyntaxToken m_openToken;
		SyntaxToken m_closeToken;

	public:
		TypeArgumentsListSyntax(gmt::Ref<SyntaxNode> parent);
		virtual ~TypeArgumentsListSyntax() = default;

		gmt::Span<const gmt::Ref<TypeSyntax>> get_types() const;
		SyntaxToken get_open_token() const;
		SyntaxToken get_close_token() const;

		void set_types(gmt::Span<gmt::Ref<TypeSyntax>> types);
		void set_open_token(const SyntaxToken& token);
		void set_close_token(const SyntaxToken& token);

		TextLocation get_location() const override;
		void accept(SyntaxVisitor& visitor) const override;
	};
}
