#pragma once
#include <shard/Definitions.hpp>

#include <cstddef>
#include <cstdint>
#include <vector>

namespace shard
{
	class MethodSymbol;
	class StructSymbol;

	struct SHARD_API FrameSlotRecipe
	{
		StructSymbol* concreteType = nullptr;
		std::int16_t typeParameterIndex = -1;
	};

	struct SHARD_API FrameLayout
	{
		static constexpr std::size_t entryHeaderSize = sizeof(void*);

		std::vector<FrameSlotRecipe> variableSlots;

		std::uint32_t maxEvalDepth = 0;
		std::size_t evalSlotPayload = 0;
		bool isComplete = false;

		// TODO: pragmatic analysis
		static std::size_t resolve_type_payload(StructSymbol* type);
		std::size_t compute_locals_bytes(const MethodSymbol& method) const;
		static std::size_t slot_payload(const MethodSymbol& method, std::uint16_t slot);
	};
}
