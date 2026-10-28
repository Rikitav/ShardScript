#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxToken.hpp>

#include <shard/parsing/nodes/TypeSyntax.hpp>
#include <shard/parsing/nodes/lists/TypeArgumentsListSyntax.hpp>

#include <gmt/Containers.hpp>
#include <gmt/Ref.hpp>

#include <string>

namespace shard
{
	class SHARD_API GenericTypeSyntax final : public TypeSyntax
	{
		TypeSyntax* m_underlayingType = nullptr;
		TypeArgumentsListSyntax* m_typeArguments = nullptr;

	public:
		GenericTypeSyntax(SyntaxNode* parent);
		virtual ~GenericTypeSyntax() = default;

		const TypeSyntax* get_underlaying_type() const;
		const TypeArgumentsListSyntax* get_type_arguments() const;

		void set_underlaying_type(TypeSyntax* underlayingType);
		void set_type_arguments(TypeArgumentsListSyntax* typeArguments);

		gmt::wstring get_qualifier() const override;

		TextLocation get_location() const override;
		void accept(SyntaxVisitor& visitor) const override;
	};
}
