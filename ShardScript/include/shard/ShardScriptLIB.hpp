#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/parsing/lexical/SourceProvider.hpp>

#include <shard/syntax/symbols/MethodSymbol.hpp>
#include <shard/syntax/symbols/AccessorSymbol.hpp>
#include <shard/syntax/symbols/ConstructorSymbol.hpp>

namespace shard
{
	struct SHARD_API ShardLibMetadata
	{
	public:
		const wchar_t* Name = nullptr;
		const wchar_t* Description = nullptr;
		const wchar_t* Version = nullptr;
	};

	class SHARD_API FrameworkModule
	{
	public:
		virtual SourceProvider* GetSource() = 0;
		virtual bool BindConstructor(ConstructorSymbol* symbol) = 0;
		virtual bool BindMethod(MethodSymbol* symbol) = 0;
		virtual bool BindAccessor(AccessorSymbol* symbol) = 0;
	};
}

#define SHARDLIB_GETMETADATA_FUNCNAME ShardLib_GetMetadata
#define SHARDLIB_ENTRYPOINT_FUNCNAME ShardLib_EntryPoint

// Microsoft
#if defined(_MSC_VER)
#define SHARDLIB_GETMETADATA extern "C" __declspec(dllexport) void SHARDLIB_GETMETADATA_FUNCNAME(ShardLibMetadata& lib)
#define SHARDLIB_ENTRYPOINT extern "C" __declspec(dllexport) void SHARDLIB_ENTRYPOINT_FUNCNAME(CompilationContext& context)

//  GCC
#elif defined(__GNUC__)
	#define SHARDLIB_GETMETADATA extern "C" __attribute__((visibility("default"))) void ShardLib_GetMetadata(ShardLibMetadata& lib)
	#define SHARDLIB_ENTRYPOINT extern "C" __attribute__((visibility("default"))) void ShardLib_EntryPoint(CompilationContext& context)
#endif