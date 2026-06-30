#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/semantic/SemanticModel.hpp>
#include <shard/semantic/symbols/MethodSymbol.hpp>

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
		inline ProgramVirtualImage& operator=(const ProgramVirtualImage& other) = delete;
	};
}
