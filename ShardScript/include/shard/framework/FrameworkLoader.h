#pragma once
#include <shard/framework/FrameworkModule.h>
#include <shard/parsing/semantic/SemanticModel.h>
#include <shard/parsing/analysis/DiagnosticsContext.h>

#include <memory>
#include <vector>

using namespace shard::parsing::semantic;

namespace shard::framework
{
	class FrameworkLoader
	{
	private:
		static std::vector<FrameworkModule*> Modules;

	public:
		static void Load(shard::parsing::semantic::SemanticModel& semanticModel, shard::parsing::analysis::DiagnosticsContext& diagnostics);

	private:
		static void LoadSingleModule(FrameworkModule* module, shard::parsing::semantic::SemanticModel& semanticModel, shard::parsing::analysis::DiagnosticsContext& diagnostics);
		static void ResolvePrmitives(shard::parsing::semantic::SemanticModel& semanticModel);
		static void ResolveGlobalMethods(shard::parsing::semantic::SemanticModel& semanticModel);
	};
}
