#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/parsing/SyntaxNode.hpp>
#include <shard/parsing/SyntaxToken.hpp>
#include <shard/parsing/SyntaxKind.hpp>

#include <shard/semantic/NamespaceTree.hpp>

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
		NamespaceNode* Namespace = nullptr;

		inline UsingDirectiveSyntax(SyntaxNode* parent)
			: SyntaxNode(SyntaxKind::UsingDirective, parent) { }

		inline UsingDirectiveSyntax(const UsingDirectiveSyntax&) = delete;

		inline virtual ~UsingDirectiveSyntax() = default;

		std::wstring ToString();
	};
}