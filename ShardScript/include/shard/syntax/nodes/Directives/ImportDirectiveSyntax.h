#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/syntax/SyntaxNode.h>
#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxToken.h>

#include <shard/syntax/nodes/ParametersListSyntax.h>
#include <shard/syntax/nodes/TypeSyntax.h>

namespace shard::syntax::nodes
{
	class SHARD_API ImportDirectiveSyntax : public SyntaxNode
	{
	public:
		SyntaxToken FromToken;
		SyntaxToken LibPathToken;
		SyntaxToken ImportToken;
		SyntaxToken IdentifierToken;
		SyntaxToken SemicolonToken;
		ParametersListSyntax* Params = nullptr;
		TypeSyntax* ReturnType = nullptr;

		inline ImportDirectiveSyntax(const SyntaxNode* parent)
			: SyntaxNode(SyntaxKind::DllImportDirective, parent) { }

		inline ImportDirectiveSyntax(const ImportDirectiveSyntax&) = delete;

		inline virtual ~ImportDirectiveSyntax()
		{
			if (Params != nullptr)
				delete Params;
		}
	};
}
