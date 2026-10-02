#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxToken.hpp>
#include <shard/parsing/SyntaxNode.hpp>

#include <shard/parsing/nodes/blocks/StatementsBlockSyntax.hpp>
#include <shard/parsing/nodes/TypeSyntax.hpp>

#include <gmt/Ref.hpp>

namespace shard
{
	class SHARD_API CatchClauseSyntax final : public SyntaxNode
	{
		SyntaxToken m_catchKeywordToken;
		SyntaxToken m_identifierToken;
		SyntaxToken m_colonToken;
		gmt::Ref<TypeSyntax> m_exceptionType;
		gmt::Ref<StatementsBlockSyntax> m_body;

	public:
		CatchClauseSyntax(gmt::Ref<SyntaxNode> parent);
		virtual ~CatchClauseSyntax() = default;

		SyntaxToken get_catch_keyword() const;
		SyntaxToken get_identifier() const;
		SyntaxToken get_colon() const;
		gmt::Ref<const TypeSyntax> get_exception_type() const;
		gmt::Ref<const StatementsBlockSyntax> get_body() const;

		void set_catch_keyword(const SyntaxToken& token);
		void set_identifier(const SyntaxToken& token);
		void set_colon(const SyntaxToken& token);
		void set_exception_type(gmt::Ref<TypeSyntax> exceptionType);
		void set_body(gmt::Ref<StatementsBlockSyntax> body);

		TextLocation get_location() const override;
		void accept(SyntaxVisitor& visitor) const override;
	};
}
