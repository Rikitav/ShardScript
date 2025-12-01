#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxNode.h>
#include <shard/syntax/SyntaxToken.h>

#include <shard/syntax/nodes/StatementsBlockSyntax.h>

#include <vector>

namespace shard::syntax::nodes
{
	class SHARD_API AccessorDeclarationSyntax : public MemberDeclarationSyntax
	{
	public:
		SyntaxToken KeywordToken;
		std::vector<SyntaxToken> Modifiers;
		SyntaxToken SemicolonToken;
		StatementsBlockSyntax* Body = nullptr;

		inline AccessorDeclarationSyntax(const SyntaxNode* parent)
			: MemberDeclarationSyntax(SyntaxKind::AccessorDeclaration, parent) { }

		inline AccessorDeclarationSyntax(const AccessorDeclarationSyntax&) = delete;

		inline ~AccessorDeclarationSyntax() override
		{
			if (Body != nullptr)
				delete Body;
		}
	};
}


