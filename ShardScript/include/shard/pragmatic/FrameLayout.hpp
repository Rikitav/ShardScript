#pragma once
#include <shard/Definitions.hpp>

#include <shard/pragmatic/PlacementVerdict.hpp>
#include <shard/pragmatic/TypeShape.hpp>

#include <gmt/Containers.hpp>
#include <gmt/Ref.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace shard
{
	class StructSymbol;

	struct SHARD_API FrameSlotRecipe
	{
		StructSymbol* concreteType = nullptr; // nullref => typeParameterIndex, or unknown
		std::int16_t typeParameterIndex = -1;
		PlacementVerdict verdict = PlacementVerdict::Heap; // stamped by the layout pass from the escape verdicts; the conservative default
	};

	// What materializing a frame needs to know about one slot: the shape, the
	// in-place storage it reserves, and the handful of predicates the runtime
	// used to re-derive from the type on every call.
	enum class FrameSlotFlag : std::uint8_t
	{
		None = 0,
		Nullable = 1 << 0,
		Open = 1 << 1,		// names a type parameter: the stored value carries its own shape
		OwnsData = 1 << 2,	// owns in-place storage inside the frame's locals region
		HeapCell = 1 << 3,	// a value variable whose storage is a collector cell
	};

	constexpr FrameSlotFlag operator|(FrameSlotFlag left, FrameSlotFlag right)
	{
		return static_cast<FrameSlotFlag>(static_cast<std::uint8_t>(left) | static_cast<std::uint8_t>(right));
	}

	constexpr bool has_flag(FrameSlotFlag value, FrameSlotFlag bit)
	{
		return (static_cast<std::uint8_t>(value) & static_cast<std::uint8_t>(bit)) != 0;
	}

	struct FrameSlotPlan
	{
		const TypeShape* shape = nullptr;	// resolved shape; null for open and unknown slots
		std::uint32_t dataBytes = 0;		// locals-region bytes reserved for this slot
		FrameSlotFlag flags = FrameSlotFlag::None;
	};

	class SHARD_API FrameLayout
	{
		gmt::vector<FrameSlotRecipe> m_variableSlots;
		gmt::vector<StructSymbol*> m_argumentTypes; // receiver first for instance methods, then parameters in order
		// resolved once by the layout pass; CallStackFrame::create used to
		// re-resolve every argument and local through the shape cache per call
		gmt::vector<const TypeShape*> m_argumentShapes;
		gmt::vector<const TypeShape*> m_variableShapes;
		// resolved lazily on the first call, which keeps generic instantiations
		// correct even when they share the open method's declaration layout
		mutable const TypeShape* m_returnShape = nullptr;
		mutable bool m_returnShapeResolved = false;
		std::uint16_t m_argumentCount = 0; // eval-stack arguments, including the implicit 'this'
		std::uint16_t m_variableCount = 0; // locals and compiler-generated temporaries
		std::uint32_t m_maxEvalDepth = 0;
		std::size_t m_evalSlotPayload = 0;
		std::size_t m_localsBytes = 0; // total frame bytes for arguments and locals; computed by the layout pass
		bool m_isComplete = false;

		// filled by build_slot_plans once the layout stops changing, so the
		// runtime never re-resolves a slot through the shape cache
		mutable gmt::vector<FrameSlotPlan> m_slotPlans;
		mutable bool m_slotPlansReady = false;
		mutable bool m_slotPlansComplete = true;

		static FrameSlotRecipe make_recipe(StructSymbol* type);

		void invalidate_slot_plans();
		void build_slot_plans() const;
		std::span<const FrameSlotPlan> ensure_slot_plans() const;
		bool slot_plans_complete() const;
		std::size_t planned_locals_bytes() const;

	public:
		static constexpr std::size_t EntryHeaderSize = sizeof(void*);

		friend class CallStackFrame;

		FrameLayout() = default;

		FrameLayout(const FrameLayout&) = delete;
		FrameLayout& operator=(const FrameLayout&) = delete;

		std::uint16_t add_variable(StructSymbol* type, PlacementVerdict verdict = PlacementVerdict::Heap);
		std::uint16_t add_temporary(StructSymbol* type, const TypeShape* shape, PlacementVerdict verdict);
		bool seal_variable(std::uint16_t variableIndex, StructSymbol* type, PlacementVerdict verdict = PlacementVerdict::Heap);
		bool set_variable_verdict(std::uint16_t variableIndex, PlacementVerdict verdict);

		void set_argument_count(std::uint16_t count);
		void add_argument(StructSymbol* type);
		const gmt::vector<StructSymbol*>& get_argument_types() const;
		std::uint16_t get_argument_count() const;
		std::uint16_t get_variable_count() const;
		std::uint16_t get_local_count() const;

		void set_argument_shapes(gmt::vector<const TypeShape*> shapes);
		void set_variable_shapes(gmt::vector<const TypeShape*> shapes);
		const TypeShape* get_argument_shape(std::size_t index) const;
		const TypeShape* get_variable_shape(std::size_t index) const;
		std::size_t get_argument_shape_count() const;
		std::size_t get_variable_shape_count() const;

		bool has_return_shape() const;
		const TypeShape* get_return_shape() const;
		void set_return_shape(const TypeShape* shape) const;

		const gmt::vector<FrameSlotRecipe>& get_variable_slots() const;

		void set_locals_bytes(std::size_t bytes);
		std::size_t get_locals_bytes() const;

		void publish_eval_layout(std::uint32_t maxDepth, std::size_t maxPayload, bool poisoned);
		std::uint32_t get_max_eval_depth() const;
		std::size_t get_eval_slot_payload() const;
		bool is_complete() const;
	
		static std::size_t resolve_payload(const TypeShape* shape, PlacementVerdict verdict);
		static std::size_t entry_bytes(std::size_t payload);

		std::size_t compute_locals_bytes(std::span<const TypeShape*> argumentShapes, std::span<const TypeShape*> variableShapes) const;
	};
}
