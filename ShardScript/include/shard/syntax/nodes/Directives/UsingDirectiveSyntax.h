#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/syntax/SyntaxNode.h>
#include <shard/syntax/SyntaxToken.h>
#include <shard/syntax/SyntaxKind.h>

#include <shard/parsing/semantic/NamespaceTree.h>

#include <vector>
#include <string>

namespace shard
{
	class SHARD_API UsingDirectiveSyntax : public SyntaxNode
	{
	public:
		SyntaxToken UsingKeywordToken;
		SyntaxToken SemicolonToken;
		std::vector<SyntaxToken> TokensList;
		shard::NamespaceNode* Namespace = nullptr;

		inline UsingDirectiveSyntax(SyntaxNode *const parent)
			: SyntaxNode(SyntaxKind::UsingDirective, parent) { }

		inline UsingDirectiveSyntax(const UsingDirectiveSyntax&) = delete;

		inline virtual ~UsingDirectiveSyntax()
		{

		}

		std::wstring ToString();
	};
}