#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxToken.hpp>

#include <shard/parsing/nodes/TypeSyntax.hpp>

#include <gmt/Ref.hpp>

#include <string>

namespace shard
{
	class SHARD_API NullableTypeSyntax final : public TypeSyntax
	{
		gmt::Ref<TypeSyntax> m_underlayingType;
		SyntaxToken m_questionToken;

	public:
		NullableTypeSyntax(gmt::Ref<SyntaxNode> parent);
		virtual ~NullableTypeSyntax() = default;

		gmt::Ref<const TypeSyntax> get_underlaying_type() const;
		SyntaxToken get_question_token() const;

		void set_underlaying_type(gmt::Ref<TypeSyntax> underlayingType);
		void set_question_token(const SyntaxToken& token);

		std::wstring get_qualifier() const override;

		TextLocation get_location() const override;
		void accept(SyntaxVisitor& visitor) const override;
	};
}
