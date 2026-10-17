#pragma once
#include <shard/Definitions.hpp>

#include <gmt/Ref.hpp>

#include <cstddef>
#include <cstdint>
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
		gmt::Ref<StructSymbol> m_baseType;
		std::vector<gmt::Ref<StructSymbol>> m_genericArguments;
		std::size_t m_size = 0;
		bool m_isReference = false; // reference storage is the baseline (spec 1.7)
		std::vector<SlotInfo> m_slots;

	public:
		TypeShape(gmt::Ref<StructSymbol> baseType, std::vector<gmt::Ref<StructSymbol>> genericArguments);

		TypeShape(const TypeShape&) = delete;
		TypeShape& operator=(const TypeShape&) = delete;

		gmt::Ref<StructSymbol> get_base_type() const;
		const std::vector<gmt::Ref<StructSymbol>>& get_generic_arguments() const;

		std::size_t get_size() const;
		bool is_reference() const;
		std::size_t get_slot_count() const;
		const SlotInfo& get_slot(std::uint32_t slotIndex) const;
	};
}
