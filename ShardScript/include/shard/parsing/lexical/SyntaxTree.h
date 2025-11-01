#pragma once
#include <shard/syntax/nodes/CompilationUnitSyntax.h>
#include <vector>

namespace shard::parsing::lexical
{
	class SyntaxTree
	{
	public:
		std::vector<shard::syntax::nodes::CompilationUnitSyntax*> CompilationUnits;

		inline SyntaxTree() { }

		inline virtual ~SyntaxTree()
		{
			for (const auto unit : CompilationUnits)
			{
				unit->~CompilationUnitSyntax();
				delete unit;
			}
		}
	};
}
