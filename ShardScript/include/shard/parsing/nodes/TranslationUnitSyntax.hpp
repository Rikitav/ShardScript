#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxNode.hpp>
#include <shard/parsing/SyntaxKind.hpp>

#include <shard/parsing/nodes/MemberDeclarationSyntax.hpp>
#include <shard/parsing/nodes/directives/UsingDirectiveSyntax.hpp>
#include <shard/parsing/nodes/directives/NamespaceDirectiveSyntax.hpp>

#include <gmt/Ref.hpp>
#include <span>

namespace shard
{
	class SHARD_API TranslationUnitSyntax final : public SyntaxNode
	{
		NamespaceDirectiveSyntax* m_namespace = nullptr;
		std::span<UsingDirectiveSyntax*> m_usings;
		std::span<MemberDeclarationSyntax*> m_members;

	public:
		TranslationUnitSyntax();
		virtual ~TranslationUnitSyntax() = default;

		TranslationUnitSyntax(const TranslationUnitSyntax&) = delete;
		TranslationUnitSyntax& operator=(const TranslationUnitSyntax&) = delete;

		bool has_namespace() const;
		const NamespaceDirectiveSyntax* get_namespace() const;
		std::span<UsingDirectiveSyntax* const> get_usings() const;
		std::span<MemberDeclarationSyntax* const> get_members() const;

		void set_namespace(NamespaceDirectiveSyntax* directive);
		void set_usings(std::span<UsingDirectiveSyntax*> usings);
		void set_members(std::span<MemberDeclarationSyntax*> members);

		TextLocation get_location() const override;
		void accept(SyntaxVisitor& visitor) const override;
	};
}