#include <shard/semantic/SemanticModel.hpp>

using namespace shard;

SemanticModel::SemanticModel(shard::SyntaxTree& tree)
	: m_tree(tree) { }

SyntaxTree& SemanticModel::get_tree() const
{
	return m_tree;
}

SymbolTable& SemanticModel::get_table()
{
	return m_table;
}
