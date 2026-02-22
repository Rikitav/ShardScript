#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/syntax/SyntaxNode.hpp>
#include <shard/syntax/SyntaxToken.hpp>
#include <shard/syntax/SyntaxKind.hpp>

#include <shard/parsing/semantic/NamespaceTree.hpp>

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