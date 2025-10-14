#pragma once
#include <shard/syntax/SyntaxNode.h>
#include <shard/syntax/SyntaxToken.h>
#include <shard/syntax/SyntaxKind.h>
#include <vector>

using namespace std;

namespace shard::syntax::nodes
{
	class UsingDirectiveSyntax : public SyntaxNode
	{
	public:
		SyntaxToken UsingKeyword;
		vector<SyntaxToken> Tokens;
		SyntaxToken Semicolon;

		UsingDirectiveSyntax()
			: SyntaxNode(SyntaxKind::UsingDirective) {}
	};
}