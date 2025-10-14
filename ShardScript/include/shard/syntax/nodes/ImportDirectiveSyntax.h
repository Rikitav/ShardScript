#pragma once
#include <shard/syntax/SyntaxNode.h>
#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxToken.h>
#include <vector>

using namespace std;

namespace shard::syntax::nodes
{
	class ImportDirectiveSyntax : public SyntaxNode
	{
	public:
		SyntaxToken FromToken;
		SyntaxToken LibPathToken;
		SyntaxToken ImportToken;
		vector<SyntaxToken> Funtctions;
		SyntaxToken Semicolon;

		ImportDirectiveSyntax()
			: SyntaxNode(SyntaxKind::DllImportDirective) {
		}
	};
}
