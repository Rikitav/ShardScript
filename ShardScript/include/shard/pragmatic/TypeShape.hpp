#pragma once
#include <shard/Definitions.hpp>

#include <gmt/Containers.hpp>
#include <gmt/Ref.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <vector>

namespace shard
{
	class StructSymbol;
	class MethodSymbol;

	class SHARD_API TypeShape
	{
		friend class TypeShapeCache;

	public:
		// What storage behind an instance holds; accessors refuse to read a kind it is not.
		enum class ValueKind : std::uint8_t
		{
			Unknown,
			Void,
			Boolean,
			Byte,
			Char,
			Integer,
			NativeInteger,
			Float,
			String,
			Enum,
			Array,
			Reference,
		};

		// Bits that do not fit in ValueKind but are needed on the same hot path.
		enum class ShapeFlag : std::uint8_t
		{
			None = 0,
			Reference = 1 << 0,
			NullableElements = 1 << 1,	// an array whose element type is nullable
		};

		struct SlotInfo
		{
			TypeShape* fieldShape; // non-owning, owned by the creating TypeShapeCache
			std::uint32_t offset;
			bool isNullable = false;
		};

	private:
		// size, kind, flags and the slot count are read on nearly every opcode;
		// the kind word stays next to the size so the group shares one cache line
		std::size_t m_size = 0;
		std::uint32_t m_kindBits = 0;
		StructSymbol* m_baseType = nullptr;
		TypeShape* m_elementShape = nullptr;

		gmt::vector<StructSymbol*> m_genericArguments;
		gmt::vector<SlotInfo> m_slots;
		gmt::vector<MethodSymbol*> m_vtable; // interface member slot -> implementation

		static constexpr unsigned ValueKindShift = 0;
		static constexpr unsigned ShapeFlagShift = 8;
		static constexpr unsigned SlotCountShift = 16;
		static constexpr unsigned SlotCountMask = 0xFFFF;

		SHARD_FORCE_INLINE void set_kind(ValueKind kind, ShapeFlag flags, std::size_t slotCount)
		{
			m_kindBits = (static_cast<std::uint32_t>(kind) << ValueKindShift)
				| (static_cast<std::uint32_t>(flags) << ShapeFlagShift)
				| ((static_cast<std::uint32_t>(slotCount) & SlotCountMask) << SlotCountShift);
		}

		SHARD_FORCE_INLINE void set_flags(ShapeFlag flags)
		{
			const std::uint32_t kept = m_kindBits & ~(0xFFu << ShapeFlagShift);
			m_kindBits = kept | (static_cast<std::uint32_t>(flags) << ShapeFlagShift);
		}

	public:
		TypeShape(StructSymbol* baseType, std::span<StructSymbol* const> genericArguments);

		TypeShape(const TypeShape&) = delete;
		TypeShape& operator=(const TypeShape&) = delete;

		SHARD_FORCE_INLINE StructSymbol* get_base_type() const { return m_baseType; }
		SHARD_FORCE_INLINE const gmt::vector<StructSymbol*>& get_generic_arguments() const { return m_genericArguments; }

		SHARD_FORCE_INLINE std::size_t get_size() const { return m_size; }
		SHARD_FORCE_INLINE const TypeShape* get_element_shape() const { return m_elementShape; }

		SHARD_FORCE_INLINE bool is_reference() const
		{
			constexpr std::uint32_t mask = static_cast<std::uint32_t>(ShapeFlag::Reference) << ShapeFlagShift;
			return (m_kindBits & mask) != 0;
		}

		SHARD_FORCE_INLINE ValueKind get_value_kind() const
		{
			return static_cast<ValueKind>((m_kindBits >> ValueKindShift) & 0xFFu);
		}

		SHARD_FORCE_INLINE bool is_enum() const { return get_value_kind() == ValueKind::Enum; }
		SHARD_FORCE_INLINE bool is_array() const { return get_value_kind() == ValueKind::Array; }

		SHARD_FORCE_INLINE bool has_nullable_elements() const
		{
			constexpr std::uint32_t mask = static_cast<std::uint32_t>(ShapeFlag::NullableElements) << ShapeFlagShift;
			return (m_kindBits & mask) != 0;
		}

		SHARD_FORCE_INLINE std::size_t get_slot_count() const { return (m_kindBits >> SlotCountShift) & SlotCountMask; }

		SHARD_FORCE_INLINE const SlotInfo& get_slot(std::uint32_t slotIndex) const
		{
			if (slotIndex >= m_slots.size())
				throw std::out_of_range("type shape slot index is out of range");

			return m_slots[slotIndex];
		}

		SHARD_FORCE_INLINE bool has_vtable() const { return !m_vtable.empty(); }
		SHARD_FORCE_INLINE std::size_t get_vtable_size() const { return m_vtable.size(); }

		SHARD_FORCE_INLINE MethodSymbol* get_vtable_entry(std::uint32_t slot) const
		{
			if (slot >= m_vtable.size())
				return nullptr;

			return m_vtable[slot];
		}

		void set_vtable(gmt::vector<MethodSymbol*> entries);
	};

	constexpr TypeShape::ShapeFlag operator|(TypeShape::ShapeFlag left, TypeShape::ShapeFlag right)
	{
		return static_cast<TypeShape::ShapeFlag>(
			static_cast<std::uint8_t>(left) | static_cast<std::uint8_t>(right));
	}

	constexpr bool has_flag(TypeShape::ShapeFlag value, TypeShape::ShapeFlag bit)
	{
		return (static_cast<std::uint8_t>(value) & static_cast<std::uint8_t>(bit)) != 0;
	}
}