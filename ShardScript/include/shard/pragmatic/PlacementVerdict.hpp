#pragma once
#include <shard/Definitions.hpp>

#include <shard/pragmatic/EscapeChannels.hpp>

namespace shard
{
	enum class PlacementVerdict
	{
		Stack,
		StackReturn, // STACK* : fresh concrete value returned to a caller frame slot
		Scalar,      // NoEscape + identity-free, dematerialized into locals
		Promote,     // born on stack; migration inserted at the cold escape point (spec 4.5)
		Heap
	};

	struct SHARD_API AllocationSiteVerdict
	{
		EscapeLevel level = EscapeLevel::NoEscape;
		EscapeChannel channels = EscapeChannel::None;
		PlacementVerdict verdict = PlacementVerdict::Stack;
	};
}
