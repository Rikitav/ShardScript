#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxToken.hpp>

#include <shard/parsing/nodes/TypeSyntax.hpp>

#include <gmt/Containers.hpp>
#include <gmt/Ref.hpp>

#include <string>

namespace shard
{
	class SHARD_API NullableTypeSyntax final : public TypeSyntax
	{
		TypeSyntax* m_underlayingType = nullptr;
		SyntaxToken m_questionToken;

	public:
		NullableTypeSyntax(SyntaxNode* parent);
		virtual ~NullableTypeSyntax() = default;

		const TypeSyntax* get_underlaying_type() const;
		SyntaxToken get_question_token() const;

		void set_underlaying_type(TypeSyntax* underlayingType);
		void set_question_token(const SyntaxToken& token);

		gmt::wstring get_qualifier() const override;

		TextLocation get_location() const override;
		void accept(SyntaxVisitor& visitor) const override;
	};
}
