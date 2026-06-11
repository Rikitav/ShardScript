#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/syntax/SyntaxNode.hpp>
#include <shard/syntax/SyntaxKind.hpp>

#include <shard/syntax/nodes/MemberDeclarationSyntax.hpp>
#include <shard/syntax/nodes/Directives/UsingDirectiveSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/NamespaceDeclarationSyntax.hpp>

#include <vector>

namespace shard
{
	enum CompilationUnitOrigin
	{
		Unknown,
		SourceFile,
		DynamicLib
	};

	class SHARD_API CompilationUnitSyntax : public SyntaxNode
	{
	public:
		CompilationUnitOrigin Origin;
		std::vector<UsingDirectiveSyntax*> Usings;
		std::vector<MemberDeclarationSyntax*> Members;
		NamespaceDeclarationSyntax* Namespace = nullptr;

		inline CompilationUnitSyntax()
			: SyntaxNode(SyntaxKind::CompilationUnit, nullptr), Origin(CompilationUnitOrigin::Unknown) { }

		inline CompilationUnitSyntax(const CompilationUnitSyntax& other) = delete;

		inline virtual ~CompilationUnitSyntax()
		{
			for (const UsingDirectiveSyntax* usingDirective : Usings)
				delete usingDirective;

			if (Namespace != nullptr)
				delete Namespace;

			for (const MemberDeclarationSyntax* member : Members)
				delete member;
		}
	};
}
