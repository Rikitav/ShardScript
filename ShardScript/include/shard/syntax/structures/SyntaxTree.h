#pragma once
#include <shard/syntax/nodes/CompilationUnitSyntax.h>
#include <shard/syntax/nodes/MethodDeclarationSyntax.h>
#include <memory>
#include <vector>

using namespace std;
using namespace shard::syntax::nodes;

namespace shard::syntax::structures
{
	class SyntaxTree
	{
	public:
		shared_ptr<MethodDeclarationSyntax> EntryPoint;
		vector<shared_ptr<CompilationUnitSyntax>> CompilationUnits;

		SyntaxTree();
	};
}
