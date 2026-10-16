#pragma once
#include <shard/Definitions.hpp>

#include <shard/pragmatic/EscapeChannels.hpp>

namespace shard
{
	// StackReturn / Scalar / Promote / Error are reserved by the spec (§1.4) for later tiers
	enum class PlacementVerdict
	{
		Stack,
		Heap
	};

	struct SHARD_API AllocationSiteVerdict
	{
		EscapeLevel level = EscapeLevel::NoEscape;
		EscapeChannel channels = EscapeChannel::None;
		PlacementVerdict verdict = PlacementVerdict::Stack;
	};
}
