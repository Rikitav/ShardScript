#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxNode.hpp>
#include <shard/parsing/SyntaxToken.hpp>

#include <shard/parsing/nodes/TypeSyntax.hpp>

#include <gmt/Ref.hpp>
#include <span>

namespace shard
{
	class SHARD_API ParameterSyntax final : public SyntaxNode
	{
		TypeSyntax* m_type = nullptr;
		SyntaxToken m_identifierToken;

	public:
		ParameterSyntax(SyntaxNode* parent);
		virtual ~ParameterSyntax() = default;

		const TypeSyntax* get_type() const;
		SyntaxToken get_identifier() const;

		void set_type(TypeSyntax* type);
		void set_identifier(const SyntaxToken& token);

		TextLocation get_location() const override;
		void accept(SyntaxVisitor& visitor) const override;
	};

	class SHARD_API ParametersListSyntax final : public SyntaxNode
	{
		std::span<ParameterSyntax*> m_parameters;

		SyntaxToken m_openToken;
		SyntaxToken m_closeToken;

	public:
		ParametersListSyntax(SyntaxNode* parent);
		virtual ~ParametersListSyntax() = default;

		std::span<ParameterSyntax* const> get_parameters() const;
		SyntaxToken get_open_token() const;
		SyntaxToken get_close_token() const;

		void set_parameters(std::span<ParameterSyntax*> parameters);
		void set_open_token(const SyntaxToken& token);
		void set_close_token(const SyntaxToken& token);

		TextLocation get_location() const override;
		void accept(SyntaxVisitor& visitor) const override;
	};
}