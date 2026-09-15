#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxToken.hpp>

#include <shard/parsing/nodes/TypeSyntax.hpp>

#include <gmt/Ref.hpp>

#include <string>

namespace shard
{
	class SHARD_API QualifiedNameTypeSyntax final : public TypeSyntax
	{
		gmt::Ref<TypeSyntax> m_left;
		SyntaxToken m_qualifierToken;
		SyntaxToken m_identifierToken;

	public:
		QualifiedNameTypeSyntax(gmt::Ref<SyntaxNode> parent);
		virtual ~QualifiedNameTypeSyntax() = default;

		gmt::Ref<const TypeSyntax> get_left() const;
		SyntaxToken get_qualifier_token() const;
		SyntaxToken get_identifier() const;

		void set_left(gmt::Ref<TypeSyntax> left);
		void set_qualifier_token(const SyntaxToken& token);
		void set_identifier(const SyntaxToken& token);

		std::wstring get_qualifier() const override;

		TextLocation get_location() const override;
		void accept(SyntaxVisitor& visitor) const override;
	};
}
