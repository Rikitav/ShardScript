#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxTree.hpp>
#include <shard/semantic/SymbolTable.hpp>

namespace shard
{
	class SHARD_API SemanticModel
	{
		shard::SyntaxTree& m_tree;
		shard::SymbolTable m_table;

	public:
		SemanticModel(shard::SyntaxTree& tree);

		SemanticModel(const SemanticModel&) = delete;
		SemanticModel& operator=(const SemanticModel&) = delete;

		SyntaxTree& get_tree() const;
		SymbolTable& get_table();
	};
}
