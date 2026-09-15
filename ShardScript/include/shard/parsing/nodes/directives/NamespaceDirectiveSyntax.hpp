#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxNode.hpp>
#include <shard/parsing/SyntaxToken.hpp>

#include <gmt/Span.hpp>

#include <string>

namespace shard
{
	class SHARD_API NamespaceDirectiveSyntax final : public SyntaxNode
	{
		gmt::Span<SyntaxToken> m_qualifier;
		SyntaxToken m_namespaceKeyword;
		SyntaxToken m_semicolon;

	public:
		NamespaceDirectiveSyntax(gmt::Ref<SyntaxNode> parent);
		virtual ~NamespaceDirectiveSyntax() = default;

		gmt::Span<const SyntaxToken> get_qualifier() const;
		std::wstring get_qualifier_string() const;
		SyntaxToken get_semicolon() const;
		SyntaxToken get_namespace_keyword() const;

		void set_qualifier(gmt::Span<SyntaxToken> qualifier);
		void set_namespace_keyword(const SyntaxToken& token);
		void set_semicolon(const SyntaxToken& token);

		TextLocation get_location() const override;
		void accept(SyntaxVisitor& visitor) const override;
	};
}
