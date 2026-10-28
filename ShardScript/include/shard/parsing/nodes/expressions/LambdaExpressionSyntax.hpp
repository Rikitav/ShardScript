#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxToken.hpp>

#include <shard/parsing/nodes/ExpressionSyntax.hpp>
#include <shard/parsing/nodes/BlockSyntax.hpp>
#include <shard/parsing/nodes/lists/ParametersListSyntax.hpp>

#include <gmt/Ref.hpp>

namespace shard
{
	class SHARD_API LambdaExpressionSyntax final : public ExpressionSyntax
	{
		ParametersListSyntax* m_parameters = nullptr;
		BlockSyntax* m_body = nullptr;

	public:
		LambdaExpressionSyntax(SyntaxNode* parent);
		virtual ~LambdaExpressionSyntax() = default;

		const ParametersListSyntax* get_parameters() const;
		const BlockSyntax* get_body() const;

		void set_parameters(ParametersListSyntax* parameters);
		void set_body(BlockSyntax* body);

		TextLocation get_location() const override;
		void accept(SyntaxVisitor& visitor) const override;
	};
}