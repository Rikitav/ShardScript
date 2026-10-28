#pragma once
#include <shard/semantic/SemanticVisitor.hpp>

#include <shard/semantic/SemanticModel.hpp>

namespace shard
{
	template<typename T>
	T* SemanticVisitor::lookup_symbol(const SyntaxNode* node)
	{
		static_assert(std::is_base_of_v<SyntaxSymbol, T>, "lookup target must derive from SyntaxSymbol");
		return static_cast<T*>(m_table->lookup_symbol(node));
	}

	template<typename T>
	std::optional<const T*> SemanticVisitor::ref_from(const T* ptr)
	{
		return m_model->get_tree().get_arena().ref_from_ptr(ptr);
	}
}