#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/FrameworkModule.hpp>
#include <shard/parsing/semantic/SemanticModel.hpp>
#include <shard/parsing/analysis/DiagnosticsContext.hpp>

#include <windows.h> // TODO: remove
#include <vector>

namespace shard
{
	/*
	class SHARD_API FrameworkLoader
	{
		static std::vector<HMODULE> LoadedLibraries;
		static std::vector<FrameworkModule*> Modules;

	public:
		static void AddLib(const std::wstring& path);
		static void AddModule(FrameworkModule* pModule);
		static void Destroy();

		static void Load(SemanticModel& semanticModel, DiagnosticsContext& diagnostics);
	};
	*/
}
