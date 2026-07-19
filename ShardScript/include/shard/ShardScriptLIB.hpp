#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/lexical/SourceProvider.hpp>

#include <shard/semantic/symbols/MethodSymbol.hpp>
#include <shard/semantic/symbols/AccessorSymbol.hpp>
#include <shard/semantic/symbols/ConstructorSymbol.hpp>

#include <cstddef>

namespace shard
{
	struct SHARD_API ShardLibDependencyInfo
	{
	public:
		const wchar_t* Name = nullptr;
		const wchar_t* VersionExpression = nullptr;
	};

	struct SHARD_API ShardLibMetadata
	{
	public:
		const wchar_t* Name = nullptr;
		const wchar_t* Description = nullptr;
		const wchar_t* Version = nullptr;
		const ShardLibDependencyInfo* Dependencies = nullptr;
		std::size_t DependenciesLength = 0;
	};
}

#define SHARDLIB_GETMETADATA_FUNCNAME ShardLib_GetMetadata
#define SHARDLIB_ENTRYPOINT_FUNCNAME ShardLib_EntryPoint

// Microsoft / MinGW
#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__) || defined(__WIN32)
	#define SHARDLIB_EXPORT __declspec(dllexport)
	#define SHARDLIB_GETMETADATA extern "C" __declspec(dllexport) void SHARDLIB_GETMETADATA_FUNCNAME(shard::ShardLibMetadata& lib)
	#define SHARDLIB_ENTRYPOINT extern "C" __declspec(dllexport) void SHARDLIB_ENTRYPOINT_FUNCNAME(shard::CompilationContext& context)

//  GCC
#elif defined(__GNUC__)
    #define SHARDLIB_EXPORT __attribute__((visibility("default")))
	#define SHARDLIB_GETMETADATA extern "C" __attribute__((visibility("default"))) void SHARDLIB_GETMETADATA_FUNCNAME(shard::ShardLibMetadata& lib)
	#define SHARDLIB_ENTRYPOINT extern "C" __attribute__((visibility("default"))) void SHARDLIB_ENTRYPOINT_FUNCNAME(shard::CompilationContext& context)

#endif
