#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxNode.hpp>
#include <shard/parsing/SyntaxToken.hpp>

#include <gmt/Ref.hpp>
#include <gmt/Span.hpp>

namespace shard
{
	class SHARD_API TypeParameterSyntax final : public SyntaxNode
	{
		SyntaxToken m_identifierToken;

	public:
		TypeParameterSyntax(gmt::Ref<SyntaxNode> parent);
		virtual ~TypeParameterSyntax() = default;

		SyntaxToken get_identifier() const;
		void set_identifier(const SyntaxToken& token);

		TextLocation get_location() const override;
		void accept(SyntaxVisitor& visitor) const override;
	};

	class SHARD_API TypeParametersListSyntax final : public SyntaxNode
	{
		gmt::Span<gmt::Ref<TypeParameterSyntax>> m_parameters;

		SyntaxToken m_openToken;
		SyntaxToken m_closeToken;

	public:
		TypeParametersListSyntax(gmt::Ref<SyntaxNode> parent);
		virtual ~TypeParametersListSyntax() = default;

		gmt::Span<const gmt::Ref<TypeParameterSyntax>> get_parameters() const;
		SyntaxToken get_open_token() const;
		SyntaxToken get_close_token() const;

		void set_parameters(gmt::Span<gmt::Ref<TypeParameterSyntax>> parameters);
		void set_open_token(const SyntaxToken& token);
		void set_close_token(const SyntaxToken& token);

		TextLocation get_location() const override;
		void accept(SyntaxVisitor& visitor) const override;
	};
}
