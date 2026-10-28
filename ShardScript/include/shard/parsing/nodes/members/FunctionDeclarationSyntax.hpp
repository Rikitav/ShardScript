#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/nodes/BlockSyntax.hpp>
#include <shard/parsing/nodes/TypeDeclarationSyntax.hpp>
#include <shard/parsing/nodes/lists/ParametersListSyntax.hpp>

#include <gmt/Ref.hpp>

namespace shard
{
	class SHARD_API FunctionDeclarationSyntax final : public TypeDeclarationSyntax
	{
		BlockSyntax* m_body = nullptr;
		TypeSyntax* m_returnType = nullptr;
		ParametersListSyntax* m_parametersList = nullptr;

	public:
		FunctionDeclarationSyntax(SyntaxNode* parent);
		virtual ~FunctionDeclarationSyntax() = default;

		const BlockSyntax* get_body() const;
		const TypeSyntax* get_return_type() const;
		const ParametersListSyntax* get_parameters_list() const;

		void set_body(BlockSyntax* body);
		void set_return_type(TypeSyntax* returnType);
		void set_parameters_list(ParametersListSyntax* parametersList);

		TextLocation get_location() const override;
		void accept(SyntaxVisitor& visitor) const override;
	};
}