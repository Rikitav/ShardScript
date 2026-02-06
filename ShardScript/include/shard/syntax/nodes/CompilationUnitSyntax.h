#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/syntax/SyntaxNode.h>
#include <shard/syntax/SyntaxKind.h>

#include <shard/syntax/nodes/MemberDeclarationSyntax.h>
#include <shard/syntax/nodes/Directives/UsingDirectiveSyntax.h>

#include <vector>

namespace shard
{
	class SHARD_API CompilationUnitSyntax : public SyntaxNode
	{
	public:
		std::vector<UsingDirectiveSyntax*> Usings;
		std::vector<MemberDeclarationSyntax*> Members;

		inline CompilationUnitSyntax()
			: SyntaxNode(SyntaxKind::CompilationUnit, nullptr) { }

		inline CompilationUnitSyntax(const CompilationUnitSyntax& other) = delete;

		inline virtual ~CompilationUnitSyntax()
		{
			for (const UsingDirectiveSyntax* usingDirective : Usings)
				delete usingDirective;

			for (const MemberDeclarationSyntax* member : Members)
				delete member;
		}
	};
}
