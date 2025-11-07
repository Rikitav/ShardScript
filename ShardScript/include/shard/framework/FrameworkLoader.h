#pragma once
#include <shard/parsing/semantic/SemanticModel.h>

using namespace shard::parsing::semantic;

namespace shard::framework
{
	class FrameworkLoader
	{
	public:
		static void Load(shard::parsing::semantic::SemanticModel& semanticModel);

	private:
		static void ResolvePrmitives(shard::parsing::semantic::SemanticModel& semanticModel);
		static void ResolveGlobalMethods(shard::parsing::semantic::SemanticModel& semanticModel);
	};
}
