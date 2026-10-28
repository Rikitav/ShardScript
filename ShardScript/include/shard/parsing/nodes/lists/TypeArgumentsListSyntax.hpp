#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxNode.hpp>
#include <shard/parsing/SyntaxToken.hpp>

#include <shard/parsing/nodes/TypeSyntax.hpp>

#include <gmt/Ref.hpp>
#include <span>

namespace shard
{
	class SHARD_API TypeArgumentsListSyntax final : public SyntaxNode
	{
		std::span<TypeSyntax*> m_types;

		SyntaxToken m_openToken;
		SyntaxToken m_closeToken;

	public:
		TypeArgumentsListSyntax(SyntaxNode* parent);
		virtual ~TypeArgumentsListSyntax() = default;

		std::span<TypeSyntax* const> get_types() const;
		SyntaxToken get_open_token() const;
		SyntaxToken get_close_token() const;

		void set_types(std::span<TypeSyntax*> types);
		void set_open_token(const SyntaxToken& token);
		void set_close_token(const SyntaxToken& token);

		TextLocation get_location() const override;
		void accept(SyntaxVisitor& visitor) const override;
	};
}