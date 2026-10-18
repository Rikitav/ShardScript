#pragma once
#include <shard/Definitions.hpp>

#include <shard/pragmatic/PlacementVerdict.hpp>
#include <shard/pragmatic/TypeShape.hpp>

#include <gmt/Ref.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace shard
{
	class StructSymbol;
	class TypeParameterSymbol;

	struct SHARD_API FrameSlotRecipe
	{
		gmt::Ref<StructSymbol> concreteType; // nullref => typeParameterIndex, or unknown
		std::int16_t typeParameterIndex = -1;
		PlacementVerdict verdict = PlacementVerdict::Heap; // stamped by the layout pass from the escape verdicts; the conservative default
	};

	class SHARD_API FrameLayout
	{
		std::vector<FrameSlotRecipe> m_variableSlots;
		std::vector<gmt::Ref<StructSymbol>> m_argumentTypes; // receiver first for instance methods, then parameters in order
		std::uint16_t m_argumentCount = 0; // eval-stack arguments, including the implicit 'this'
		std::uint16_t m_variableCount = 0; // locals and compiler-generated temporaries
		std::uint32_t m_maxEvalDepth = 0;
		std::size_t m_evalSlotPayload = 0;
		std::size_t m_localsBytes = 0; // total frame bytes for arguments and locals; computed by the layout pass
		bool m_isComplete = false;

		static FrameSlotRecipe make_recipe(gmt::Ref<StructSymbol> type, const std::vector<gmt::Ref<TypeParameterSymbol>>& typeParameters);

	public:
		static constexpr std::size_t EntryHeaderSize = sizeof(void*);

		FrameLayout() = default;

		FrameLayout(const FrameLayout&) = delete;
		FrameLayout& operator=(const FrameLayout&) = delete;

		std::uint16_t add_variable(gmt::Ref<StructSymbol> type, const std::vector<gmt::Ref<TypeParameterSymbol>>& typeParameters, PlacementVerdict verdict = PlacementVerdict::Heap);
		bool seal_variable(std::uint16_t variableIndex, gmt::Ref<StructSymbol> type, const std::vector<gmt::Ref<TypeParameterSymbol>>& typeParameters, PlacementVerdict verdict = PlacementVerdict::Heap);
		bool set_variable_verdict(std::uint16_t variableIndex, PlacementVerdict verdict);

		void set_argument_count(std::uint16_t count);
		void add_argument(gmt::Ref<StructSymbol> type);
		const std::vector<gmt::Ref<StructSymbol>>& get_argument_types() const;
		std::uint16_t get_argument_count() const;
		std::uint16_t get_variable_count() const;
		std::uint16_t get_local_count() const;

		const std::vector<FrameSlotRecipe>& get_variable_slots() const;

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
