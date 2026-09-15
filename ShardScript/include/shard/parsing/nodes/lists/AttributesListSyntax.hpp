#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxToken.hpp>
#include <shard/parsing/SyntaxNode.hpp>
#include <gmt/Span.hpp>

namespace shard
{
	class SHARD_API AttributeSyntax final : public SyntaxNode
	{
		SyntaxToken m_openBracketToken;
		SyntaxToken m_closeBracketToken;
		SyntaxToken m_nameToken;
		SyntaxToken m_openCurlToken;
		SyntaxToken m_closeCurlToken;
		gmt::Span<SyntaxToken> m_arguments;

	public:
		AttributeSyntax(gmt::Ref<SyntaxNode> parent);
		virtual ~AttributeSyntax() = default;

		void set_open_bracket(const SyntaxToken& token);
		void set_close_bracket(const SyntaxToken& token);
		void set_name(const SyntaxToken& token);
		void set_open_curl(const SyntaxToken& token);
		void set_close_curl(const SyntaxToken& token);
		void set_arguments(gmt::Span<SyntaxToken> arguments);

		SyntaxToken get_open_bracket() const;
		SyntaxToken get_close_bracket() const;
		SyntaxToken get_name() const;
		SyntaxToken get_open_curl() const;
		SyntaxToken get_close_curl() const;
		gmt::Span<const SyntaxToken> get_arguments() const;

		TextLocation get_location() const override;
		void accept(SyntaxVisitor& visitor) const override;
	};

	class SHARD_API AttributesListSyntax final : public SyntaxNode
	{
		gmt::Span<gmt::Ref<AttributeSyntax>> m_attributes;

		SyntaxToken m_openToken;
		SyntaxToken m_closeToken;

	public:
		AttributesListSyntax(gmt::Ref<SyntaxNode> parent);
		virtual ~AttributesListSyntax() = default;

		gmt::Span<const gmt::Ref<AttributeSyntax>> get_attributes() const;
		SyntaxToken get_open_token() const;
		SyntaxToken get_close_token() const;

		void set_attributes(gmt::Span<gmt::Ref<AttributeSyntax>> attributes);
		void set_open_token(const SyntaxToken& token);
		void set_close_token(const SyntaxToken& token);

		TextLocation get_location() const override;
		void accept(SyntaxVisitor& visitor) const override;
	};
}
