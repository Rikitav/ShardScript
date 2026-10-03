#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxToken.hpp>

#include <shard/parsing/nodes/MemberDeclarationSyntax.hpp>
#include <shard/parsing/nodes/BodySyntax.hpp>
#include <shard/parsing/nodes/lists/ParametersListSyntax.hpp>

#include <gmt/Ref.hpp>

namespace shard
{
	class SHARD_API ConstructorDeclarationSyntax final : public MemberDeclarationSyntax
	{
		gmt::Ref<ParametersListSyntax> m_parametersList;
		gmt::Ref<BodySyntax> m_body;
		SyntaxToken m_semicolonToken;

	public:
		ConstructorDeclarationSyntax(gmt::Ref<SyntaxNode> parent);
		virtual ~ConstructorDeclarationSyntax() = default;

		gmt::Ref<const ParametersListSyntax> get_parameters_list() const;
		gmt::Ref<const BodySyntax> get_body() const;
		SyntaxToken get_semicolon() const;

		void set_parameters_list(gmt::Ref<ParametersListSyntax> parametersList);
		void set_body(gmt::Ref<BodySyntax> body);
		void set_semicolon(const SyntaxToken& token);

		TextLocation get_location() const override;
		void accept(SyntaxVisitor& visitor) const override;
	};
}
