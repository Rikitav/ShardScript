#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxNode.hpp>
#include <shard/parsing/SyntaxToken.hpp>

#include <shard/parsing/nodes/TypeSyntax.hpp>

#include <gmt/Ref.hpp>
#include <gmt/Span.hpp>

namespace shard
{
	class SHARD_API BaseTypesListSyntax final : public SyntaxNode
	{
		gmt::Span<gmt::Ref<TypeSyntax>> m_types;

	public:
		BaseTypesListSyntax(gmt::Ref<SyntaxNode> parent);
		virtual ~BaseTypesListSyntax() = default;

		gmt::Span<const gmt::Ref<TypeSyntax>> get_types() const;
		void set_types(gmt::Span<gmt::Ref<TypeSyntax>> types);

		TextLocation get_location() const override;
		void accept(SyntaxVisitor& visitor) const override;
	};
}
