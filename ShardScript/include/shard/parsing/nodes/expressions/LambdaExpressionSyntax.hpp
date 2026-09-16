#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxToken.hpp>

#include <shard/parsing/nodes/ExpressionSyntax.hpp>
#include <shard/parsing/nodes/BodySyntax.hpp>
#include <shard/parsing/nodes/lists/ParametersListSyntax.hpp>

#include <gmt/Ref.hpp>

namespace shard
{
	class SHARD_API LambdaExpressionSyntax final : public ExpressionSyntax
	{
		gmt::Ref<ParametersListSyntax> m_parameters;
		gmt::Ref<BodySyntax> m_body;

	public:
		LambdaExpressionSyntax(gmt::Ref<SyntaxNode> parent);
		virtual ~LambdaExpressionSyntax() = default;

		gmt::Ref<const ParametersListSyntax> get_parameters() const;
		gmt::Ref<const BodySyntax> get_body() const;

		void set_parameters(gmt::Ref<ParametersListSyntax> parameters);
		void set_body(gmt::Ref<BodySyntax> body);

		TextLocation get_location() const override;
		void accept(SyntaxVisitor& visitor) const override;
	};
}
