#pragma once
#include <shard/ShardScriptAPI.h>
#include <shard/syntax/nodes/CompilationUnitSyntax.h>
#include <vector>

namespace shard::parsing::lexical
{
	class SHARD_API SyntaxTree
	{
	public:
		std::vector<shard::syntax::nodes::CompilationUnitSyntax*> CompilationUnits;

		inline SyntaxTree() { }

		inline virtual ~SyntaxTree()
		{
			for (const auto unit : CompilationUnits)
				delete unit;
		}
	};
}
