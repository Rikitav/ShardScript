#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxNode.hpp>
#include <shard/parsing/SyntaxKind.hpp>

#include <shard/parsing/nodes/MemberDeclarationSyntax.hpp>
#include <shard/parsing/nodes/directives/UsingDirectiveSyntax.hpp>
#include <shard/parsing/nodes/directives/NamespaceDirectiveSyntax.hpp>

#include <gmt/Ref.hpp>
#include <gmt/Span.hpp>

namespace shard
{
	class SHARD_API TranslationUnitSyntax final : public SyntaxNode
	{
		gmt::Ref<NamespaceDirectiveSyntax> m_namespace;
		gmt::Span<gmt::Ref<UsingDirectiveSyntax>> m_usings;
		gmt::Span<gmt::Ref<MemberDeclarationSyntax>> m_members;

	public:
		TranslationUnitSyntax();
		virtual ~TranslationUnitSyntax() = default;

		TranslationUnitSyntax(const TranslationUnitSyntax&) = delete;
		TranslationUnitSyntax& operator=(const TranslationUnitSyntax&) = delete;

		bool has_namespace() const;
		gmt::Ref<const NamespaceDirectiveSyntax> get_namespace() const;
		gmt::Span<const gmt::Ref<UsingDirectiveSyntax>> get_usings() const;
		gmt::Span<const gmt::Ref<MemberDeclarationSyntax>> get_members() const;

		void set_namespace(gmt::Ref<NamespaceDirectiveSyntax> directive);
		void set_usings(gmt::Span<gmt::Ref<UsingDirectiveSyntax>> usings);
		void set_members(gmt::Span<gmt::Ref<MemberDeclarationSyntax>> members);

		TextLocation get_location() const override;
		void accept(SyntaxVisitor& visitor) const override;
	};
}
