#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxToken.hpp>

#include <shard/parsing/nodes/TypeSyntax.hpp>
#include <shard/parsing/nodes/lists/TypeArgumentsListSyntax.hpp>

#include <gmt/Ref.hpp>

#include <string>

namespace shard
{
	class SHARD_API GenericTypeSyntax final : public TypeSyntax
	{
		gmt::Ref<TypeSyntax> m_underlayingType;
		gmt::Ref<TypeArgumentsListSyntax> m_typeArguments;

	public:
		GenericTypeSyntax(gmt::Ref<SyntaxNode> parent);
		virtual ~GenericTypeSyntax() = default;

		gmt::Ref<const TypeSyntax> get_underlaying_type() const;
		gmt::Ref<const TypeArgumentsListSyntax> get_type_arguments() const;

		void set_underlaying_type(gmt::Ref<TypeSyntax> underlayingType);
		void set_type_arguments(gmt::Ref<TypeArgumentsListSyntax> typeArguments);

		std::wstring get_qualifier() const override;

		TextLocation get_location() const override;
		void accept(SyntaxVisitor& visitor) const override;
	};
}
