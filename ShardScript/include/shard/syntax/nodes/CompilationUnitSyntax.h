#pragma once
#include <shard/syntax/SyntaxNode.h>
#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/nodes/MemberDeclarationSyntax.h>
#include <shard/syntax/nodes/UsingDirectiveSyntax.h>
#include <shard/syntax/nodes/ImportDirectiveSyntax.h>
#include <shard/syntax/analysis/DiagnosticsContext.h>
#include <vector>
#include <memory>

using namespace std;

namespace shard::syntax::nodes
{
	class CompilationUnitSyntax : public SyntaxNode
	{
	public:
		vector<shared_ptr<ImportDirectiveSyntax>> Imports;
		vector<shared_ptr<UsingDirectiveSyntax>> Usings;
		vector<shared_ptr<MemberDeclarationSyntax>> Members;

		CompilationUnitSyntax()
			: SyntaxNode(SyntaxKind::CompilationUnit), Usings(), Members() {}
	};
}
