#pragma once
#include <shard/Definitions.hpp>

#include <shard/pragmatic/TypeShape.hpp>

#include <gmt/Containers.hpp>
#include <gmt/Ref.hpp>

#include <cstddef>
#include <unordered_map>
#include <span>
#include <vector>

namespace shard
{
	class SemanticModel;
	class StructSymbol;

	class SHARD_API TypeShapeCache
	{
		gmt::unordered_map<std::uint64_t, TypeShape> m_shapes;

		void build(TypeShape* shape, SemanticModel& model);

	public:
		TypeShapeCache() = default;
		~TypeShapeCache() = default;

		TypeShapeCache(const TypeShapeCache&) = delete;
		TypeShapeCache& operator=(const TypeShapeCache&) = delete;

		TypeShape* find_type_shape(SemanticModel& model, gmt::Ref<StructSymbol> baseType, std::span<const gmt::Ref<StructSymbol>> genericArgs = {}) const;
		TypeShape* find_or_create_type_shape(SemanticModel& model, gmt::Ref<StructSymbol> baseType, std::span<const gmt::Ref<StructSymbol>> genericArgs = {});
	};
}
