#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxToken.hpp>

#include <shard/parsing/nodes/TypeSyntax.hpp>

#include <gmt/Containers.hpp>

#include <string>

namespace shard
{
	class SHARD_API IdentifierNameTypeSyntax final : public TypeSyntax
	{
		SyntaxToken m_identifierToken;

	public:
		IdentifierNameTypeSyntax(SyntaxNode* parent);
		virtual ~IdentifierNameTypeSyntax() = default;

		SyntaxToken get_identifier() const;
		void set_identifier(const SyntaxToken& token);

		gmt::wstring get_qualifier() const override;

		TextLocation get_location() const override;
		void accept(SyntaxVisitor& visitor) const override;
	};
}
