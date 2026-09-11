#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxNode.hpp>
#include <shard/parsing/SyntaxKind.hpp>

#include <shard/parsing/nodes/MemberDeclarationSyntax.hpp>
#include <shard/parsing/nodes/directives/UsingDirectiveSyntax.hpp>
#include <shard/parsing/nodes/directives/NamespaceDirectiveSyntax.hpp>

#include <vector>
#include <optional>
#include <memory>

namespace shard
{
	class SHARD_API TranslationUnitSyntax : public SyntaxNode
	{
	public:
		NamespaceDirectiveSyntax* m_namespace;
		std::vector<UsingDirectiveSyntax*> m_usings;
		std::vector<MemberDeclarationSyntax*> m_members;

		TranslationUnitSyntax();
		virtual ~TranslationUnitSyntax() = default;

		TranslationUnitSyntax(const TranslationUnitSyntax&) = delete;
		TranslationUnitSyntax& operator=(const TranslationUnitSyntax&) = delete;

		const NamespaceDirectiveSyntax& get_namespace() const;
		std::span<const UsingDirectiveSyntax*> get_usings() const;
		std::span<const MemberDeclarationSyntax*> get_members() const;

		void add_using(UsingDirectiveSyntax* directive);
		void set_namespace(NamespaceDirectiveSyntax* directive);
		void add_member(MemberDeclarationSyntax* member);

		virtual TextLocation get_location() const override;
		void accept(SyntaxVisitor& visitor) const override;
	};
}
