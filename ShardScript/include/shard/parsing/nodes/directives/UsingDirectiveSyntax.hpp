#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxNode.hpp>
#include <shard/parsing/SyntaxToken.hpp>

#include <gmt/Span.hpp>

#include <string>

namespace shard
{
	class SHARD_API UsingDirectiveSyntax final : public SyntaxNode
	{
		gmt::Span<SyntaxToken> m_qualifier;
		SyntaxToken m_usingKeyword;
		SyntaxToken m_semicolon;

	public:
		UsingDirectiveSyntax(gmt::Ref<SyntaxNode> parent);
		virtual ~UsingDirectiveSyntax() = default;

		gmt::Span<const SyntaxToken> get_qualifier() const;
		std::wstring get_qualifier_string() const;
		SyntaxToken get_semicolon() const;
		SyntaxToken get_using_keyword() const;

		void set_qualifier(gmt::Span<SyntaxToken> qualifier);
		void set_using_keyword(const SyntaxToken& token);
		void set_semicolon(const SyntaxToken& token);

		TextLocation get_location() const override;
		void accept(SyntaxVisitor& visitor) const override;
	};
}
