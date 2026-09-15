#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/nodes/TypeDeclarationSyntax.hpp>

#include <gmt/Ref.hpp>

namespace shard
{
	class SHARD_API ClassDeclarationSyntax final : public TypeDeclarationSyntax
	{
	public:
		ClassDeclarationSyntax(gmt::Ref<SyntaxNode> parent);
		virtual ~ClassDeclarationSyntax() = default;

		TextLocation get_location() const override;
		void accept(SyntaxVisitor& visitor) const override;
	};
}
