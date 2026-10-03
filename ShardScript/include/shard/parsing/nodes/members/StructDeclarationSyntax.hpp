#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/nodes/TypeDeclarationSyntax.hpp>

#include <gmt/Ref.hpp>

namespace shard
{
	class SHARD_API StructDeclarationSyntax final : public TypeDeclarationSyntax
	{
	public:
		StructDeclarationSyntax(gmt::Ref<SyntaxNode> parent);
		virtual ~StructDeclarationSyntax() = default;

		TextLocation get_location() const override;
		void accept(SyntaxVisitor& visitor) const override;
	};
}
