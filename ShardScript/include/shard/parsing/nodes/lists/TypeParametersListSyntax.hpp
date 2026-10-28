#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxNode.hpp>
#include <shard/parsing/SyntaxToken.hpp>

#include <gmt/Ref.hpp>
#include <span>

namespace shard
{
	class SHARD_API TypeParameterSyntax final : public SyntaxNode
	{
		SyntaxToken m_identifierToken;

	public:
		TypeParameterSyntax(SyntaxNode* parent);
		virtual ~TypeParameterSyntax() = default;

		SyntaxToken get_identifier() const;
		void set_identifier(const SyntaxToken& token);

		TextLocation get_location() const override;
		void accept(SyntaxVisitor& visitor) const override;
	};

	class SHARD_API TypeParametersListSyntax final : public SyntaxNode
	{
		std::span<TypeParameterSyntax*> m_parameters;

		SyntaxToken m_openToken;
		SyntaxToken m_closeToken;

	public:
		TypeParametersListSyntax(SyntaxNode* parent);
		virtual ~TypeParametersListSyntax() = default;

		std::span<TypeParameterSyntax* const> get_parameters() const;
		SyntaxToken get_open_token() const;
		SyntaxToken get_close_token() const;

		void set_parameters(std::span<TypeParameterSyntax*> parameters);
		void set_open_token(const SyntaxToken& token);
		void set_close_token(const SyntaxToken& token);

		TextLocation get_location() const override;
		void accept(SyntaxVisitor& visitor) const override;
	};
}