#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxNode.h>
#include <shard/syntax/SyntaxToken.h>

#include <shard/syntax/nodes/StatementsBlockSyntax.h>

#include <vector>

namespace shard
{
	class SHARD_API AccessorDeclarationSyntax : public MemberDeclarationSyntax
	{
	public:
		SyntaxToken KeywordToken;
		std::vector<SyntaxToken> Modifiers;
		SyntaxToken SemicolonToken;
		StatementsBlockSyntax* Body = nullptr;

		inline AccessorDeclarationSyntax(SyntaxNode *const parent)
			: MemberDeclarationSyntax(SyntaxKind::AccessorDeclaration, parent) { }

		inline AccessorDeclarationSyntax(SyntaxNode *const parent, StatementsBlockSyntax* body)
			: MemberDeclarationSyntax(SyntaxKind::AccessorDeclaration, parent), Body(body) { }

		inline AccessorDeclarationSyntax(const AccessorDeclarationSyntax&) = delete;

		inline ~AccessorDeclarationSyntax() override
		{

		}
	};
}


