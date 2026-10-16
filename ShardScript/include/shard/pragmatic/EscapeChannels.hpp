#pragma once
#include <shard/Definitions.hpp>

namespace shard
{
	enum class EscapeLevel : unsigned int
	{
		NoEscape = 0,
		ArgEscape = 1,
		GlobalEscape = 2
	};

	enum class EscapeChannel : unsigned int
	{
		None = 0,
		Return = 1,         // E1
		Store = 2,          // E2
		Capture = 4,        // E3
		Suspension = 8,     // E4
		UnknownCallee = 16, // E5
		Exception = 32      // E7
	};

	SHARD_API EscapeLevel join_escape_level(EscapeLevel left, EscapeLevel right);

	SHARD_API EscapeChannel operator|(EscapeChannel left, EscapeChannel right);
	SHARD_API EscapeChannel operator&(EscapeChannel left, EscapeChannel right);
}
