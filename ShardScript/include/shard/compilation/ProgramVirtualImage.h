#pragma once
#include <shard/parsing/semantic/SemanticModel.h>

#include <vector>

namespace shard
{
	class ProgramVirtualImage
	{
		SemanticModel& SemanticModel;
		MethodSymbol* EntryPoint;
		std::vector<std::byte> DataSection;
	};
}
