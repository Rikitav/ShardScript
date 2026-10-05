#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxToken.hpp>

#include <shard/parsing/nodes/MemberDeclarationSyntax.hpp>
#include <shard/parsing/nodes/members/EnumFieldDeclarationSyntax.hpp>

#include <gmt/Ref.hpp>
#include <gmt/Span.hpp>

namespace shard
{
	class SHARD_API EnumDeclarationSyntax final : public MemberDeclarationSyntax
	{
		SyntaxToken m_colonToken;
		SyntaxToken m_underlyingTypeToken;
		bool m_isFlags;

		SyntaxToken m_openBracketToken;
		SyntaxToken m_closeBracketToken;
		SyntaxToken m_semicolonToken;

		gmt::Span<gmt::Ref<EnumFieldDeclarationSyntax>> m_fields;

	public:
		EnumDeclarationSyntax(gmt::Ref<SyntaxNode> parent);
		virtual ~EnumDeclarationSyntax() = default;

		SyntaxToken get_colon() const;
		SyntaxToken get_underlying_type() const;
		bool get_is_flags() const;
		SyntaxToken get_open_bracket() const;
		SyntaxToken get_close_bracket() const;
		SyntaxToken get_semicolon() const;
		gmt::Span<const gmt::Ref<EnumFieldDeclarationSyntax>> get_fields() const;

		void set_colon(const SyntaxToken& token);
		void set_underlying_type(const SyntaxToken& token);
		void set_is_flags(bool isFlags);
		void set_open_bracket(const SyntaxToken& token);
		void set_close_bracket(const SyntaxToken& token);
		void set_semicolon(const SyntaxToken& token);
		void set_fields(gmt::Span<gmt::Ref<EnumFieldDeclarationSyntax>> fields);

		TextLocation get_location() const override;
		void accept(SyntaxVisitor& visitor) const override;
	};
}
