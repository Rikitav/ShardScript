#pragma once
#include <shard/Definitions.hpp>
#include <shard/runtime/MethodCallState.hpp>

#include <cstddef>

namespace shard
{
	typedef void (*MethodSymbolDelegate)(MethodCallState& context);

	struct SHARD_API ShardLibDependencyInfo
	{
	public:
		string_t Name = nullptr;
		string_t VersionExpression = nullptr;
	};

	struct SHARD_API ShardLibMetadata
	{
	public:
		string_t Name = nullptr;
		string_t Description = nullptr;
		string_t Version = nullptr;
		const ShardLibDependencyInfo* Dependencies = nullptr;
		std::size_t DependenciesLength = 0;
	};
}

#define SHARDLIB_GETMETADATA_FUNCNAME ShardLib_GetMetadata
#define SHARDLIB_ENTRYPOINT_FUNCNAME ShardLib_EntryPoint

#define SHARDLIB_GETMETADATA_FUNCNAME_STR SHARD_STRINGIFY(SHARDLIB_GETMETADATA_FUNCNAME)
#define SHARDLIB_ENTRYPOINT_FUNCNAME_STR SHARD_STRINGIFY(SHARDLIB_ENTRYPOINT_FUNCNAME)

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

#else
	#error Unknown vendor, please define SHARDLIB_EXPORT, SHARDLIB_GETMETADATA, and SHARDLIB_ENTRYPOINT for your compiler.

#endif
