#pragma once
#include <shard/Definitions.hpp>

#include <shard/runtime/ObjectInstance.hpp>

#include <gmt/Ref.hpp>

#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace shard
{
	class TypeShape;
	class FieldSymbol;
	class SemanticModel;
	class PragmaticModel;
	class CallStackFrame;
	class StructSymbol;

	class SHARD_API GarbageCollector
	{
	public:
		static constexpr std::uint64_t Magic = 0x5348415244474348ULL;

		struct GcHeader
		{
			std::uint64_t magic;
			const TypeShape* shape;
			std::uint64_t marked;
			std::uint64_t payloadBytes;
		};

	private:
		struct StaticStorage
		{
			const TypeShape* shape;
			std::byte* storage;
		};

		SemanticModel& m_model;
		PragmaticModel& m_pragmatic;

		std::unordered_set<std::byte*> m_heap;
		std::unordered_map<std::size_t, StaticStorage> m_statics;
		std::map<std::wstring, ObjectInstance, std::less<>> m_internedStrings;
		std::vector<CallStackFrame*> m_frames;
		std::vector<ObjectInstance> m_pinned;

		std::size_t m_allocatedSinceCollection = 0;
		std::size_t m_collectionThreshold = 256 * 1024;

		std::size_t m_totalAllocations = 0;
		std::size_t m_totalAllocatedBytes = 0;
		std::size_t m_totalCollections = 0;

		ObjectInstance allocate_block(const TypeShape* shape, std::size_t payloadBytes);
		void mark_object(std::byte* payload);
		void trace_pointer(std::byte* pointer);
		void trace_object_fields(const TypeShape* shape, std::byte* payload);
		void trace_frame(CallStackFrame* frame);
		void sweep();

	public:
		GarbageCollector(SemanticModel& model, PragmaticModel& pragmatic);
		~GarbageCollector();

		GarbageCollector(const GarbageCollector&) = delete;
		GarbageCollector& operator=(const GarbageCollector&) = delete;

		ObjectInstance allocate_instance(const TypeShape* shape);
		ObjectInstance allocate_instance(const TypeShape* shape, std::size_t payloadBytes);
		ObjectInstance allocate_array(const TypeShape* arrayShape, const TypeShape* elementShape, std::int64_t length);
		ObjectInstance allocate_string(std::wstring_view value);
		ObjectInstance intern_string(std::wstring_view value);

		ObjectInstance get_static_storage(gmt::Ref<FieldSymbol> field);

		void register_frame(CallStackFrame* frame);
		void unregister_frame(CallStackFrame* frame);

		void pin(ObjectInstance instance);
		void unpin(std::size_t count);

		void collect();
		void terminate();

		std::size_t get_instance_count() const;
		std::size_t get_total_allocations() const;
		std::size_t get_total_allocated_bytes() const;
		std::size_t get_total_collections() const;
		bool is_heap_backed(std::byte* payload) const;

		static GcHeader* get_header(std::byte* payload);
		static bool is_heap_pointer(std::byte* pointer);
	};
}
