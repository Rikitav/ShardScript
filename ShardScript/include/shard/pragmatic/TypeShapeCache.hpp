#pragma once
#include <shard/Definitions.hpp>

#include <shard/pragmatic/TypeShape.hpp>

#include <gmt/Containers.hpp>
#include <gmt/Ref.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace shard
{
	class SemanticModel;
	class StructSymbol;

	// Shapes outlive every value that points at them, so they live in a stable
	// contiguous store and are addressed through a flat open-addressed index
	// instead of one heap node per shape.
	class SHARD_API TypeShapeCache
	{
		// nodes never move once pushed, so a TypeShape* handed to a caller (or
		// registered by a native plugin) stays valid for the cache's lifetime
		gmt::deque<TypeShape> m_shapes;

		// open-addressed key index: 0 means empty, otherwise shape index + 1
		gmt::vector<std::uint32_t> m_buckets;
		std::size_t m_count = 0;

		// last-hit memo; the same handful of shapes repeat on nearly every
		// opcode, so a direct-mapped line avoids the table probe entirely
		static constexpr std::size_t MemoSize = 8;
		mutable const TypeShape* m_memo[MemoSize] = {};

		static std::size_t hash_key(const void* key);
		std::size_t probe(StructSymbol* keyType) const;
		void rehash(std::size_t capacity);
		void insert(StructSymbol* keyType);
		void remember(const TypeShape* shape) const;

		void build(TypeShape* shape, SemanticModel& model);

	public:
		TypeShapeCache();
		~TypeShapeCache() = default;

		TypeShapeCache(const TypeShapeCache&) = delete;
		TypeShapeCache& operator=(const TypeShapeCache&) = delete;

		TypeShape* find_type_shape(SemanticModel& model, StructSymbol* baseType, std::span<StructSymbol* const> genericArgs = {}) const;
		TypeShape* find_or_create_type_shape(SemanticModel& model, StructSymbol* baseType, std::span<StructSymbol* const> genericArgs = {});
	};
}