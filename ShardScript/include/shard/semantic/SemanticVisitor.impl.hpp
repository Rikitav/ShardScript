#pragma once
#include <shard/semantic/SemanticVisitor.hpp>

#include <shard/semantic/SemanticModel.hpp>

namespace shard
{
	template<typename T>
	gmt::Ref<T> SemanticVisitor::lookup_symbol(gmt::Ref<const SyntaxNode> node)
	{
		static_assert(std::is_base_of_v<SyntaxSymbol, T>, "lookup target must derive from SyntaxSymbol");
		return m_table->lookup_symbol(node).as<T>();
	}

	template<typename T>
	std::optional<gmt::Ref<const T>> SemanticVisitor::ref_from(const T* ptr)
	{
		return m_model->get_tree().get_arena().ref_from_ptr(ptr);
	}
}
