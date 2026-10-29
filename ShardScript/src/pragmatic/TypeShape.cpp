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
