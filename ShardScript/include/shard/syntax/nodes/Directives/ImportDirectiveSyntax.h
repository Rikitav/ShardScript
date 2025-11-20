#pragma once
#include <shard/syntax/SyntaxNode.h>
#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxToken.h>
#include <vector>

namespace shard::syntax::nodes
{
	class ImportDirectiveSyntax : public SyntaxNode
	{
	public:
		SyntaxToken FromToken;
		SyntaxToken LibPathToken;
		SyntaxToken ImportToken;
		std::vector<SyntaxToken> FunctionsList;
		SyntaxToken SemicolonToken;

		inline ImportDirectiveSyntax(const SyntaxNode* parent)
			: SyntaxNode(SyntaxKind::DllImportDirective, parent) { }

		inline ImportDirectiveSyntax(const ImportDirectiveSyntax& other)
			: SyntaxNode(other), FromToken(other.FromToken), LibPathToken(other.LibPathToken), ImportToken(other.ImportToken), SemicolonToken(other.SemicolonToken) { }

		inline virtual ~ImportDirectiveSyntax()
		{

		}
	};
}
