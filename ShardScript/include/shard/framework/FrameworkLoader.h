#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/framework/FrameworkModule.h>
#include <shard/parsing/semantic/SemanticModel.h>
#include <shard/parsing/analysis/DiagnosticsContext.h>

#include <Windows.h>
#include <vector>

using namespace shard::parsing::semantic;

namespace shard::framework
{
	class SHARD_API FrameworkLoader
	{
	private:
		static std::vector<HMODULE> LoadedLibraries;
		static std::vector<FrameworkModule*> Modules;

	public:
		static void AddLib(const std::wstring& path);
		static void AddModule(FrameworkModule* pModule);
		static void Destroy();

		static void Load(shard::parsing::semantic::SemanticModel& semanticModel, shard::parsing::analysis::DiagnosticsContext& diagnostics);
	};
}
