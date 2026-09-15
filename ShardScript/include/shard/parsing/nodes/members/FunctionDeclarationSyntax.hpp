#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/nodes/BodySyntax.hpp>
#include <shard/parsing/nodes/TypeDeclarationSyntax.hpp>
#include <shard/parsing/nodes/lists/ParametersListSyntax.hpp>

#include <gmt/Ref.hpp>

namespace shard
{
	class SHARD_API FunctionDeclarationSyntax final : public TypeDeclarationSyntax
	{
		gmt::Ref<BodySyntax> m_body;
		gmt::Ref<TypeSyntax> m_returnType;
		gmt::Ref<ParametersListSyntax> m_parametersList;

	public:
		FunctionDeclarationSyntax(gmt::Ref<SyntaxNode> parent);
		virtual ~FunctionDeclarationSyntax() = default;

		gmt::Ref<const BodySyntax> get_body() const;
		gmt::Ref<const TypeSyntax> get_return_type() const;
		gmt::Ref<const ParametersListSyntax> get_parameters_list() const;

		void set_body(gmt::Ref<BodySyntax> body);
		void set_return_type(gmt::Ref<TypeSyntax> returnType);
		void set_parameters_list(gmt::Ref<ParametersListSyntax> parametersList);

		TextLocation get_location() const override;
		void accept(SyntaxVisitor& visitor) const override;
	};
}
