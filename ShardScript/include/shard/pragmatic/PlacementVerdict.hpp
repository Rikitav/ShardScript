#pragma once
#include <shard/Definitions.hpp>

#include <shard/pragmatic/EscapeChannels.hpp>

namespace shard
{
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
