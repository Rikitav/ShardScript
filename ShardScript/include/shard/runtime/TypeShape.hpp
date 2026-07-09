#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/semantic/symbols/TypeSymbol.hpp>

#include <vector>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>

namespace shard
{
	class SHARD_API TypeShape
	{
	public:
		TypeSymbol* BaseType;
		std::vector<TypeSymbol*> GenericArguments;

		struct SlotInfo
		{
			std::size_t Offset;
			TypeShape* FieldShape;
		};

		std::size_t Size;
		std::vector<SlotInfo> Slots;

		inline TypeShape(TypeSymbol* baseType, std::vector<TypeSymbol*> genericArguments)
			: BaseType(baseType), GenericArguments(std::move(genericArguments)), Size(0)
		{
		}

		TypeShape(const TypeShape&) = delete;
		TypeShape& operator=(const TypeShape&) = delete;

		[[nodiscard]] inline std::size_t GetOffset(const std::uint32_t slotIndex) const
		{
			if (slotIndex >= Slots.size())
				throw std::out_of_range("TypeShape slot index is out of range");

			return Slots[slotIndex].Offset;
		}

		[[nodiscard]] inline TypeShape* GetFieldShape(const std::uint32_t slotIndex) const
		{
			if (slotIndex >= Slots.size())
				throw std::out_of_range("TypeShape slot index is out of range");

			return Slots[slotIndex].FieldShape;
		}

		[[nodiscard]] inline bool IsReferenceType() const
		{
			return BaseType->Inlining == TypeInlining::ByReference;
		}

		[[nodiscard]] inline bool HasGenericArguments() const
		{
			return !GenericArguments.empty();
		}
	};
}
