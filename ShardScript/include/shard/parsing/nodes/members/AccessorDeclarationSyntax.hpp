#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxToken.hpp>

#include <shard/parsing/nodes/MemberDeclarationSyntax.hpp>
#include <shard/parsing/nodes/BlockSyntax.hpp>

namespace shard
{
	class SHARD_API AccessorDeclarationSyntax final : public MemberDeclarationSyntax
	{
		SyntaxToken m_keywordToken;
		SyntaxToken m_semicolonToken;
		BlockSyntax* m_body = nullptr;

	public:
		AccessorDeclarationSyntax(SyntaxNode* parent);
		virtual ~AccessorDeclarationSyntax() = default;

		SyntaxToken get_keyword() const;
		SyntaxToken get_semicolon() const;
		const BlockSyntax* get_body() const;

		void set_keyword(const SyntaxToken& token);
		void set_semicolon(const SyntaxToken& token);
		void set_body(BlockSyntax* body);

		TextLocation get_location() const override;
		void accept(SyntaxVisitor& visitor) const override;
	};
}