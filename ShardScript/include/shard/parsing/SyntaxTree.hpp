#pragma once
#include <shard/ShardScriptAPI.hpp>
#include <shard/syntax/nodes/CompilationUnitSyntax.hpp>
#include <vector>

namespace shard
{
	class SHARD_API SyntaxTree
	{
	public:
		std::vector<shard::CompilationUnitSyntax*> CompilationUnits;

		inline SyntaxTree() { }

		inline virtual ~SyntaxTree()
		{
			for (const auto unit : CompilationUnits)
				delete unit;
		}
	};
}
