#pragma once
#include <shard/syntax/SyntaxNode.h>
#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/nodes/MemberDeclarationSyntax.h>
#include <shard/syntax/nodes/Directives/UsingDirectiveSyntax.h>
#include <shard/syntax/nodes/Directives/ImportDirectiveSyntax.h>
#include <vector>

namespace shard::syntax::nodes
{
	class CompilationUnitSyntax : public SyntaxNode
	{
	public:
		std::vector<ImportDirectiveSyntax*> Imports;
		std::vector<UsingDirectiveSyntax*> Usings;
		std::vector<MemberDeclarationSyntax*> Members;

		inline CompilationUnitSyntax() : SyntaxNode(SyntaxKind::CompilationUnit, nullptr)
		{

		}

		inline CompilationUnitSyntax(const CompilationUnitSyntax& other)
			: SyntaxNode(other), Imports(other.Imports), Usings(other.Usings), Members(other.Members) { }

		inline virtual ~CompilationUnitSyntax()
		{
			for (const ImportDirectiveSyntax* import : Imports)
			{
				import->~ImportDirectiveSyntax();
				delete import;
			}

			for (const UsingDirectiveSyntax* usingDirective : Usings)
			{
				usingDirective->~UsingDirectiveSyntax();
				delete usingDirective;
			}

			for (const MemberDeclarationSyntax* member : Members)
			{
				member->~MemberDeclarationSyntax();
				delete member;
			}

			Imports.~vector();
			Usings.~vector();
			Members.~vector();
		}
	};
}
