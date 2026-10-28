#pragma once
#include <shard/Definitions.hpp>

#include <gmt/Containers.hpp>
#include <gmt/Ref.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace shard
{
	class StructSymbol;

	class SHARD_API TypeShape
	{
		friend class TypeShapeCache;

	public:
		struct SlotInfo
		{
			std::size_t offset;
			TypeShape* fieldShape; // non-owning, owned by the creating TypeShapeCache
		};

	private:
		StructSymbol* m_baseType = nullptr;
		gmt::vector<StructSymbol*> m_genericArguments;
		std::size_t m_size = 0;
		bool m_isReference = false; // reference storage is the baseline (spec 1.7)
		gmt::vector<SlotInfo> m_slots;

	public:
		TypeShape(StructSymbol* baseType, std::span<StructSymbol* const> genericArguments);

		TypeShape(const TypeShape&) = delete;
		TypeShape& operator=(const TypeShape&) = delete;

		StructSymbol* get_base_type() const;
		const gmt::vector<StructSymbol*>& get_generic_arguments() const;

		std::size_t get_size() const;
		bool is_reference() const;
		std::size_t get_slot_count() const;
		const SlotInfo& get_slot(std::uint32_t slotIndex) const;
	};
}
