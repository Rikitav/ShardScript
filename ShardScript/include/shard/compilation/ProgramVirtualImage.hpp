#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/semantic/SemanticModel.hpp>
#include <shard/semantic/symbols/MethodSymbol.hpp>
#include <shard/runtime/TypeShapeCache.hpp>

#include <vector>
#include <memory>

namespace shard
{
	class SHARD_API ProgramVirtualImage
	{
	public:
		MethodSymbol* EntryPoint = nullptr;
		std::vector<std::byte> DataSection;
		std::unique_ptr<TypeShapeCache> TypeShapes;

		inline ProgramVirtualImage() : TypeShapes(std::make_unique<TypeShapeCache>()) { }
		inline ProgramVirtualImage(const ProgramVirtualImage& other) = delete;
		inline ProgramVirtualImage& operator=(const ProgramVirtualImage& other) = delete;
	};
}
