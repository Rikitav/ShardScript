#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxToken.hpp>

#include <shard/parsing/nodes/TypeSyntax.hpp>

#include <string>

namespace shard
{
	class SHARD_API PredefinedTypeSyntax final : public TypeSyntax
	{
		SyntaxToken m_typeToken;

	public:
		PredefinedTypeSyntax(gmt::Ref<SyntaxNode> parent);
		virtual ~PredefinedTypeSyntax() = default;

		SyntaxToken get_type_token() const;
		void set_type_token(const SyntaxToken& token);

		std::wstring get_qualifier() const override;

		TextLocation get_location() const override;
		void accept(SyntaxVisitor& visitor) const override;
	};
}
