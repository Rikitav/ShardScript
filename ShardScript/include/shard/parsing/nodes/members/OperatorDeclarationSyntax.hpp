#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxToken.hpp>

#include <shard/parsing/nodes/MemberDeclarationSyntax.hpp>
#include <shard/parsing/nodes/BlockSyntax.hpp>
#include <shard/parsing/nodes/TypeSyntax.hpp>
#include <shard/parsing/nodes/lists/ParametersListSyntax.hpp>

#include <gmt/Ref.hpp>

namespace shard
{
	class SHARD_API OperatorDeclarationSyntax final : public MemberDeclarationSyntax
	{
		SyntaxToken m_operatorToken;
		gmt::Ref<ParametersListSyntax> m_parametersList;
		gmt::Ref<TypeSyntax> m_returnType;
		gmt::Ref<BlockSyntax> m_body;
		SyntaxToken m_semicolonToken;

	public:
		OperatorDeclarationSyntax(gmt::Ref<SyntaxNode> parent);
		virtual ~OperatorDeclarationSyntax() = default;

		SyntaxToken get_operator_token() const;
		gmt::Ref<const ParametersListSyntax> get_parameters_list() const;
		gmt::Ref<const TypeSyntax> get_return_type() const;
		gmt::Ref<const BlockSyntax> get_body() const;
		SyntaxToken get_semicolon() const;

		void set_operator_token(const SyntaxToken& token);
		void set_parameters_list(gmt::Ref<ParametersListSyntax> parametersList);
		void set_return_type(gmt::Ref<TypeSyntax> returnType);
		void set_body(gmt::Ref<BlockSyntax> body);
		void set_semicolon(const SyntaxToken& token);

		TextLocation get_location() const override;
		void accept(SyntaxVisitor& visitor) const override;
	};
}
