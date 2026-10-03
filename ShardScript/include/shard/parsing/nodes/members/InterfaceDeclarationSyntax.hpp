#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/nodes/TypeDeclarationSyntax.hpp>

#include <gmt/Ref.hpp>

namespace shard
{
	class SHARD_API InterfaceDeclarationSyntax final : public TypeDeclarationSyntax
	{
	public:
		InterfaceDeclarationSyntax(gmt::Ref<SyntaxNode> parent);
		virtual ~InterfaceDeclarationSyntax() = default;

		TextLocation get_location() const override;
		void accept(SyntaxVisitor& visitor) const override;
	};
}
