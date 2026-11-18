#pragma once
#include <shard/Definitions.hpp>

namespace shard
{
	// how a native callee may retain a value handed to it; drives stack placement
	enum class EscapeContract
	{
		Global,			// may end up anywhere reachable after the call
		WithInstance,	// may only be stored into the receiver
		OnlyReads		// never stored, captured or returned by the callee
	};
}
