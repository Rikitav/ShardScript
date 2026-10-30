#pragma once
#include <shard/Definitions.hpp>

#include <shard/runtime/ObjectInstance.hpp>

#include <gmt/Containers.hpp>
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

		// Open-addressed live-object table. The previous std::unordered_set
		// allocated a list node for every GC object, doubling the allocator
		// traffic of allocate_block; slots make insert/erase allocation-free
		// between rehashes.
		class HeapTable
		{
			static std::byte* const Tombstone;

			gmt::vector<std::byte*> m_slots;
			std::size_t m_size = 0;
			std::size_t m_tombstones = 0;

			std::size_t home(std::byte* value) const;
			std::size_t locate(std::byte* value) const;
			void rehash(std::size_t capacity);

		public:
			HeapTable();

			void insert(std::byte* value);
			bool erase(std::byte* value);
			bool contains(std::byte* value) const;
			void clear();
			std::size_t size() const;

			template<typename F>
			void for_each(F&& visitor) const
			{
				for (std::byte* slot : m_slots)
				{
					if (slot != nullptr && slot != Tombstone)
						visitor(slot);
				}
			}

			// calls free_block(payload); removing the payload when it returns true
			template<typename F>
			void sweep(F&& free_block)
			{
				for (std::byte*& slot : m_slots)
				{
					if (slot == nullptr || slot == Tombstone)
						continue;

					if (free_block(slot))
					{
						slot = Tombstone;
						--m_size;
						++m_tombstones;
					}
				}
			}
		};

		SemanticModel& m_model;
		PragmaticModel& m_pragmatic;

		HeapTable m_heap;
		gmt::unordered_map<std::size_t, StaticStorage> m_statics;
		gmt::map<gmt::wstring, ObjectInstance, std::less<>> m_internedStrings;
		gmt::vector<CallStackFrame*> m_frames;
		gmt::vector<ObjectInstance> m_pinned;
		gmt::vector<ObjectInstance> m_roots;

		std::size_t m_allocatedSinceCollection = 0;
		std::size_t m_collectionThreshold = 256 * 1024;

		std::size_t m_totalAllocations = 0;
		std::size_t m_totalAllocatedBytes = 0;
		std::size_t m_totalCollections = 0;

		// Explicit mark stack. The previous recursive walk overflowed the
		// native stack on a retained chain of ~11k objects (STATUS_STACK_OVERFLOW);
		// a worklist has no depth limit and keeps marking iterative.
		gmt::vector<std::byte*> m_markStack;

		ObjectInstance allocate_block(const TypeShape* shape, std::size_t payloadBytes);
		void mark_object(std::byte* payload);
		void trace_pointer(std::byte* pointer);
		void drain_mark_stack();
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

		ObjectInstance get_static_storage(FieldSymbol* field);

		void register_frame(CallStackFrame* frame);
		void unregister_frame(CallStackFrame* frame);

		void pin(ObjectInstance instance);
		void unpin(std::size_t count);

		// persistent roots for objects whose lifetime the VM tracks outside
		// any frame or static slot (pending task states, continuations)
		void add_root(ObjectInstance instance);
		void remove_root(ObjectInstance instance);

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
