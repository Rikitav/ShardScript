#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/parsing/semantic/SemanticModel.hpp>
#include <shard/syntax/symbols/MethodSymbol.hpp>

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
