#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/syntax/SyntaxNode.h>
#include <shard/syntax/SyntaxKind.h>

#include <shard/syntax/nodes/MemberDeclarationSyntax.h>
#include <shard/syntax/nodes/Directives/UsingDirectiveSyntax.h>
#include <shard/syntax/nodes/Directives/ImportDirectiveSyntax.h>

#include <vector>

namespace shard::syntax::nodes
{
	class SHARD_API CompilationUnitSyntax : public SyntaxNode
	{
	public:
		std::vector<ImportDirectiveSyntax*> Imports;
		std::vector<UsingDirectiveSyntax*> Usings;
		std::vector<MemberDeclarationSyntax*> Members;

		inline CompilationUnitSyntax() : SyntaxNode(SyntaxKind::CompilationUnit, nullptr)
		{

		}

		inline CompilationUnitSyntax(const CompilationUnitSyntax& other) = delete;

		inline virtual ~CompilationUnitSyntax()
		{
			for (const ImportDirectiveSyntax* import : Imports)
				delete import;

			for (const UsingDirectiveSyntax* usingDirective : Usings)
				delete usingDirective;

			for (const MemberDeclarationSyntax* member : Members)
				delete member;
		}
	};
}
