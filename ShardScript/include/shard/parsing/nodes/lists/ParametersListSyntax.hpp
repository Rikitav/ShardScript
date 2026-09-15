#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxNode.hpp>
#include <shard/parsing/SyntaxToken.hpp>

#include <shard/parsing/nodes/TypeSyntax.hpp>

#include <gmt/Ref.hpp>
#include <gmt/Span.hpp>

namespace shard
{
	class SHARD_API ParameterSyntax final : public SyntaxNode
	{
		gmt::Ref<TypeSyntax> m_type;
		SyntaxToken m_identifierToken;

	public:
		ParameterSyntax(gmt::Ref<SyntaxNode> parent);
		virtual ~ParameterSyntax() = default;

		gmt::Ref<const TypeSyntax> get_type() const;
		SyntaxToken get_identifier() const;

		void set_type(gmt::Ref<TypeSyntax> type);
		void set_identifier(const SyntaxToken& token);

		TextLocation get_location() const override;
		void accept(SyntaxVisitor& visitor) const override;
	};

	class SHARD_API ParametersListSyntax final : public SyntaxNode
	{
		gmt::Span<gmt::Ref<ParameterSyntax>> m_parameters;

		SyntaxToken m_openToken;
		SyntaxToken m_closeToken;

	public:
		ParametersListSyntax(gmt::Ref<SyntaxNode> parent);
		virtual ~ParametersListSyntax() = default;

		gmt::Span<const gmt::Ref<ParameterSyntax>> get_parameters() const;
		SyntaxToken get_open_token() const;
		SyntaxToken get_close_token() const;

		void set_parameters(gmt::Span<gmt::Ref<ParameterSyntax>> parameters);
		void set_open_token(const SyntaxToken& token);
		void set_close_token(const SyntaxToken& token);

		TextLocation get_location() const override;
		void accept(SyntaxVisitor& visitor) const override;
	};
}
