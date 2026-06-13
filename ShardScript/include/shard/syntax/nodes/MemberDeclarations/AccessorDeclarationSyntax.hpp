#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/syntax/SyntaxKind.hpp>
#include <shard/syntax/SyntaxNode.hpp>
#include <shard/syntax/SyntaxToken.hpp>

#include <shard/syntax/nodes/StatementsBlockSyntax.hpp>

#include <vector>
#include <memory>

namespace shard
{
	class SHARD_API AccessorDeclarationSyntax : public MemberDeclarationSyntax
	{
	public:
		SyntaxToken KeywordToken;
		std::vector<SyntaxToken> Modifiers;
		SyntaxToken SemicolonToken;
		std::unique_ptr<StatementsBlockSyntax> Body = nullptr;

		inline AccessorDeclarationSyntax(SyntaxNode *const parent)
			: MemberDeclarationSyntax(SyntaxKind::AccessorDeclaration, parent) { }

		inline AccessorDeclarationSyntax(SyntaxNode *const parent, std::unique_ptr<StatementsBlockSyntax> body)
			: MemberDeclarationSyntax(SyntaxKind::AccessorDeclaration, parent), Body(std::move(body)) { }

		inline AccessorDeclarationSyntax(const AccessorDeclarationSyntax&) = delete;

		inline ~AccessorDeclarationSyntax() override = default;
	};
}
