#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxNode.hpp>
#include <shard/parsing/SyntaxToken.hpp>

#include <shard/parsing/nodes/TypeSyntax.hpp>

#include <gmt/Ref.hpp>
#include <span>

namespace shard
{
	class SHARD_API BaseTypesListSyntax final : public SyntaxNode
	{
		std::span<TypeSyntax*> m_types;

	public:
		BaseTypesListSyntax(SyntaxNode* parent);
		virtual ~BaseTypesListSyntax() = default;

		std::span<TypeSyntax* const> get_types() const;
		void set_types(std::span<TypeSyntax*> types);

		TextLocation get_location() const override;
		void accept(SyntaxVisitor& visitor) const override;
	};
}