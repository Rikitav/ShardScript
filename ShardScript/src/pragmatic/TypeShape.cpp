#include <shard/pragmatic/TypeShape.hpp>

#include <shard/semantic/symbols/StructSymbol.hpp>

#include <gmt/Containers.hpp>

#include <stdexcept>

using namespace shard;

TypeShape::TypeShape(StructSymbol* baseType, std::span<StructSymbol* const> genericArguments)
	: m_baseType(baseType)
{
	m_genericArguments.assign(genericArguments.begin(), genericArguments.end());
}

StructSymbol* TypeShape::get_base_type() const
{
	return m_baseType;
}

const gmt::vector<StructSymbol*>& TypeShape::get_generic_arguments() const
{
	return m_genericArguments;
}

std::size_t TypeShape::get_size() const
{
	return m_size;
}

const TypeShape* TypeShape::get_element_shape() const
{
	return m_elementShape;
}

bool TypeShape::is_reference() const
{
	return m_isReference;
}

std::size_t TypeShape::get_slot_count() const
{
	return m_slots.size();
}

const TypeShape::SlotInfo& TypeShape::get_slot(std::uint32_t slotIndex) const
{
	if (slotIndex >= m_slots.size())
		throw std::out_of_range("type shape slot index is out of range");

	return m_slots[slotIndex];
}

bool TypeShape::has_vtable() const
{
	return !m_vtable.empty();
}

std::size_t TypeShape::get_vtable_size() const
{
	return m_vtable.size();
}

MethodSymbol* TypeShape::get_vtable_entry(std::uint32_t slot) const
{
	if (slot >= m_vtable.size())
		return nullptr;

	return m_vtable[slot];
}

void TypeShape::set_vtable(gmt::vector<MethodSymbol*> entries)
{
	m_vtable = std::move(entries);
}
