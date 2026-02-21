#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/syntax/SyntaxNode.hpp>
#include <shard/syntax/SyntaxKind.hpp>

#include <shard/syntax/nodes/MemberDeclarationSyntax.hpp>
#include <shard/syntax/nodes/Directives/UsingDirectiveSyntax.hpp>

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
