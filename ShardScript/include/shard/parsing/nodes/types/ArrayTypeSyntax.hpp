#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxToken.hpp>

#include <shard/parsing/nodes/TypeSyntax.hpp>

#include <gmt/Ref.hpp>

#include <string>

namespace shard
{
	class SHARD_API ArrayTypeSyntax final : public TypeSyntax
	{
		gmt::Ref<TypeSyntax> m_underlayingType;
		SyntaxToken m_openBracketToken;
		SyntaxToken m_closeBracketToken;

	public:
		ArrayTypeSyntax(gmt::Ref<SyntaxNode> parent);
		virtual ~ArrayTypeSyntax() = default;

		gmt::Ref<const TypeSyntax> get_underlaying_type() const;
		SyntaxToken get_open_bracket() const;
		SyntaxToken get_close_bracket() const;

		void set_underlaying_type(gmt::Ref<TypeSyntax> underlayingType);
		void set_open_bracket(const SyntaxToken& token);
		void set_close_bracket(const SyntaxToken& token);

		std::wstring get_qualifier() const override;

		TextLocation get_location() const override;
		void accept(SyntaxVisitor& visitor) const override;
	};
}
