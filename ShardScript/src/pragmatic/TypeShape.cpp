#include <shard/pragmatic/TypeShape.hpp>

#include <shard/semantic/symbols/StructSymbol.hpp>

#include <gmt/Containers.hpp>

using namespace shard;

TypeShape::TypeShape(StructSymbol* baseType, std::span<StructSymbol* const> genericArguments)
	: m_baseType(baseType)
{
	m_genericArguments.assign(genericArguments.begin(), genericArguments.end());
}

void TypeShape::set_vtable(gmt::vector<MethodSymbol*> entries)
{
	m_vtable = std::move(entries);
}