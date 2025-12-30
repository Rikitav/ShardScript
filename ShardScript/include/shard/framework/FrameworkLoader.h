#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/framework/FrameworkModule.h>
#include <shard/parsing/semantic/SemanticModel.h>
#include <shard/parsing/analysis/DiagnosticsContext.h>

#include <Windows.h>
#include <vector>

namespace shard
{
	class SHARD_API FrameworkLoader
	{
	private:
		static std::vector<HMODULE> LoadedLibraries;
		static std::vector<shard::FrameworkModule*> Modules;

	public:
		static void AddLib(const std::wstring& path);
		static void AddModule(shard::FrameworkModule* pModule);
		static void Destroy();

		static void Load(shard::SemanticModel& semanticModel, shard::DiagnosticsContext& diagnostics);
	};
}
