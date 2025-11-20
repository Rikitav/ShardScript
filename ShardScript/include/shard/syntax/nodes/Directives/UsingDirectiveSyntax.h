#pragma once
#include <shard/syntax/SyntaxNode.h>
#include <shard/syntax/SyntaxToken.h>
#include <shard/syntax/SyntaxKind.h>
#include <vector>

namespace shard::syntax::nodes
{
	class UsingDirectiveSyntax : public SyntaxNode
	{
	public:
		SyntaxToken UsingKeywordToken;
		SyntaxToken SemicolonToken;
		std::vector<SyntaxToken> TokensList;

		inline UsingDirectiveSyntax(const SyntaxNode* parent)
			: SyntaxNode(SyntaxKind::UsingDirective, parent) { }

		inline UsingDirectiveSyntax(const UsingDirectiveSyntax& other)
			: SyntaxNode(other), UsingKeywordToken(other.UsingKeywordToken), SemicolonToken(other.SemicolonToken) { }

		inline virtual ~UsingDirectiveSyntax()
		{

		}
	};
}