#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxToken.hpp>

#include <shard/parsing/nodes/TypeDeclarationSyntax.hpp>
#include <shard/parsing/nodes/TypeSyntax.hpp>
#include <shard/parsing/nodes/lists/ParametersListSyntax.hpp>

#include <gmt/Ref.hpp>

namespace shard
{
	class SHARD_API DelegateDeclarationSyntax final : public TypeDeclarationSyntax
	{
		gmt::Ref<ParametersListSyntax> m_parametersList;
		gmt::Ref<TypeSyntax> m_returnType;
		SyntaxToken m_semicolonToken;

	public:
		DelegateDeclarationSyntax(gmt::Ref<SyntaxNode> parent);
		virtual ~DelegateDeclarationSyntax() = default;

		gmt::Ref<const ParametersListSyntax> get_parameters_list() const;
		gmt::Ref<const TypeSyntax> get_return_type() const;
		SyntaxToken get_semicolon() const;

		void set_parameters_list(gmt::Ref<ParametersListSyntax> parametersList);
		void set_return_type(gmt::Ref<TypeSyntax> returnType);
		void set_semicolon(const SyntaxToken& token);

		TextLocation get_location() const override;
		void accept(SyntaxVisitor& visitor) const override;
	};
}
