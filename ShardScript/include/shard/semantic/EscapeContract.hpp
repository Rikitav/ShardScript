#pragma once
#include <shard/Definitions.hpp>

namespace shard
{
	// how a native callee may retain a value handed to it; drives stack placement
	enum class EscapeContract
	{
		Global,			// may end up anywhere reachable after the call
		WithInstance,	// only stored into the receiver, which bounds its lifetime
		OnlyReads		// never stored, captured or returned, and never hands an element out
	};
}
