#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/parsing/semantic/SemanticModel.h>
#include <shard/syntax/symbols/MethodSymbol.h>

#include <vector>

namespace shard
{
	class SHARD_API ProgramVirtualImage
	{
	public:
		MethodSymbol* EntryPoint = nullptr;
		std::vector<std::byte> DataSection;

		inline ProgramVirtualImage() { }
		inline ProgramVirtualImage(const ProgramVirtualImage& other) = delete;
	};
}
