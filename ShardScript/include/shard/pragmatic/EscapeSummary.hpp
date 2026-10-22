#pragma once
#include <shard/Definitions.hpp>

#include <shard/pragmatic/EscapeChannels.hpp>

#include <gmt/Containers.hpp>
#include <gmt/Ref.hpp>

#include <vector>

namespace shard
{
	class StructSymbol;

	struct SHARD_API ParameterEscapeSummary
	{
		EscapeChannel channels = EscapeChannel::None;		// strongest hot flows observed for this parameter
		EscapeChannel coldChannels = EscapeChannel::None;	// upward flows observed only on cold (exceptional) paths
		bool escapes = false;								// any hot upward flow (E1/E2/E3/E7 or unknown-callee E5)
	};

	enum class ReturnDisposition
	{
		Unknown,			// unresolved / interface / delegate / unknown body
		Fresh,				// returns a value created in the method (caller-placeable if concrete)
		ReturnsParameter	// returns a parameter (or an alias chain rooted in one)
	};

	struct SHARD_API EscapeSummary
	{
		gmt::vector<ParameterEscapeSummary> parameters;
		ReturnDisposition returnDisposition = ReturnDisposition::Unknown;
		gmt::Ref<StructSymbol> returnType;   // nullref when Unknown
		bool computed = false;               // false = conservative default (unknown callee)
	};
}
