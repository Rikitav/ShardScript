#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxNode.hpp>
#include <shard/parsing/SyntaxToken.hpp>
#include <shard/parsing/SyntaxKind.hpp>
#include <gmt/Ref.hpp>
#include <gmt/Span.hpp>

#include <shard/parsing/nodes/AttributeSyntax.hpp>

namespace shard
{
	class SHARD_API MemberDeclarationSyntax : public SyntaxNode
	{
		gmt::Span<gmt::Ref<AttributeSyntax>> m_attributes;
		gmt::Span<SyntaxToken> m_modifiers;
		SyntaxToken m_identifierToken;

	public:
		MemberDeclarationSyntax(const SyntaxKind kind, gmt::Ref<SyntaxNode> parent);
		virtual ~MemberDeclarationSyntax() = default;

		void set_attributes(gmt::Span<gmt::Ref<AttributeSyntax>> attributes);
		void set_modifiers(gmt::Span<SyntaxToken> modifiers);
		void set_identifier(const SyntaxToken& identifier);

		gmt::Span<const gmt::Ref<AttributeSyntax>> get_attributes() const;
		gmt::Span<const SyntaxToken> get_modifiers() const;
		SyntaxToken get_identifier() const;

		virtual TextLocation get_location() const override;
		void accept(SyntaxVisitor& visitor) const override;
	};
}
