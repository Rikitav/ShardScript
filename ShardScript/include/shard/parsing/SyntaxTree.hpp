#pragma once
#include <shard/ShardScriptAPI.hpp>
#include <shard/parsing/nodes/CompilationUnitSyntax.hpp>

#include <vector>
#include <memory>

namespace shard
{
	class SHARD_API SyntaxTree
	{
	public:
		std::vector<std::unique_ptr<shard::CompilationUnitSyntax>> CompilationUnits;

		inline SyntaxTree() = default;
		inline virtual ~SyntaxTree() = default;

		SyntaxTree(const SyntaxTree&) = delete;
		SyntaxTree& operator=(const SyntaxTree&) = delete;
	};
}
