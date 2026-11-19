#pragma once
#include <shard/Definitions.hpp>

#include <shard/pragmatic/TypeShape.hpp>
#include <shard/runtime/ObjectInstance.hpp>

#include <gmt/Containers.hpp>
#include <gmt/Ref.hpp>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <vector>

namespace shard
{
	class MethodSymbol;
	class FrameLayout;
	class SemanticModel;
	class PragmaticModel;
	class GarbageCollector;

	enum class InterruptionReason
	{
		None,
		ValueReturned,
		ExceptionRaised,
		Awaiting
	};

	struct ExceptionHandlerRecord
	{
		std::size_t handlerOffset;
		std::size_t deferStackBase;
		std::size_t evalDepthBase;
	};

	struct FrameEntryView
	{
		const TypeShape* shape;
		bool isReferenceStorage;
		std::byte* payloadAddress;
	};

	class SHARD_API FrameBlockPool
	{
		static constexpr std::size_t MaxClasses = 32;
		std::byte* m_heads[MaxClasses] = {};

	public:
		FrameBlockPool() = default;
		~FrameBlockPool();

		FrameBlockPool(const FrameBlockPool&) = delete;
		FrameBlockPool& operator=(const FrameBlockPool&) = delete;

		std::byte* acquire(std::size_t bytes);
		void release(std::byte* block, std::size_t bytes);
		void release_all();

	private:
		static std::size_t class_of(std::size_t bytes);
	};

	class SHARD_API CallStackFrame
	{
	public:
		static constexpr std::size_t EntryHeaderSize = sizeof(std::uintptr_t);
		static constexpr std::uintptr_t ReferenceTag = 1;

	private:
		MethodSymbol* m_method = nullptr;
		CallStackFrame* m_previous = nullptr;
		GarbageCollector* m_collector = nullptr;

		std::byte* m_evalRegion = nullptr;
		std::size_t m_evalStride = 0;
		std::uint32_t m_evalCapacity = 0;
		std::uint32_t m_evalDepth = 0;
		InterruptionReason m_interruption = InterruptionReason::None;
		std::uint16_t m_argumentCount = 0;
		std::uint16_t m_variableCount = 0;

		struct LocalSlot
		{
			const TypeShape* shape = nullptr;
			std::byte* dataAddress = nullptr;
			bool isNullable = false;
			bool isOpen = false;	// names a type parameter: the stored value owns the shape
		};

		LocalSlot* m_localSlots = nullptr;
		std::byte** m_variableSlots = nullptr;
		std::uint16_t m_localCount = 0;
		std::byte* m_localsRegion = nullptr;

		const TypeShape* m_returnShape = nullptr;
		std::byte* m_returnDestination = nullptr;
		ObjectInstance m_returnResult;
		std::size_t m_blockBytes = 0;
		const FrameLayout* m_layout = nullptr;
		std::size_t m_instructionPointer = 0;
		ObjectInstance m_currentException;

		std::uint32_t m_pendingChildren = 0;
		std::byte* m_asyncState = nullptr;
		std::byte* m_awaitedState = nullptr;
		bool m_suspendedAtReturn = false;
		bool m_runnable = false;

		gmt::vector<std::size_t> m_deferStack;
		gmt::vector<ExceptionHandlerRecord> m_exceptionHandlers;

		CallStackFrame() = default;
		
		SHARD_FORCE_INLINE static std::uintptr_t pack_header(const TypeShape* shape, bool isReferenceStorage)
		{
			std::uintptr_t header = reinterpret_cast<std::uintptr_t>(shape);
			if (isReferenceStorage)
				header |= ReferenceTag;

			return header;
		}

		SHARD_FORCE_INLINE static const TypeShape* unpack_shape(std::uintptr_t header)
		{
			return reinterpret_cast<const TypeShape*>(header & ~ReferenceTag);
		}

		SHARD_FORCE_INLINE static bool unpack_is_reference(std::uintptr_t header)
		{
			return (header & ReferenceTag) != 0;
		}

		SHARD_FORCE_INLINE FrameEntryView entry_view(std::byte* entry) const
		{
			return peek_entry(entry);
		}

		SHARD_FORCE_INLINE std::byte* entry_at(std::size_t depth) const
		{
			return m_evalRegion + m_evalStride * depth;
		}

		// materializes a slot whose storage has to come from the collector
		std::byte* allocate_local_cell(std::uint16_t slot);

	public:
		SHARD_FORCE_INLINE static FrameEntryView peek_entry(std::byte* entry)
		{
			std::uintptr_t header;
			std::memcpy(&header, entry, EntryHeaderSize);

			FrameEntryView view{};
			view.shape = unpack_shape(header);
			view.isReferenceStorage = unpack_is_reference(header);
			view.payloadAddress = entry + EntryHeaderSize;
			return view;
		}

		static CallStackFrame* create(MethodSymbol* method, const FrameLayout* layout, SemanticModel& model, PragmaticModel& pragmatic, CallStackFrame* previous, GarbageCollector* collector = nullptr, std::byte* externalReturnDestination = nullptr, std::size_t externalReturnPayload = 0, FrameBlockPool* pool = nullptr);
		static void destroy(CallStackFrame* frame, FrameBlockPool* pool = nullptr);

		CallStackFrame(const CallStackFrame&) = delete;
		CallStackFrame& operator=(const CallStackFrame&) = delete;

		SHARD_FORCE_INLINE MethodSymbol* get_method() const { return m_method; }
		SHARD_FORCE_INLINE CallStackFrame* get_previous() const { return m_previous; }

		SHARD_FORCE_INLINE std::size_t get_instruction_pointer() const { return m_instructionPointer; }
		SHARD_FORCE_INLINE void set_instruction_pointer(std::size_t offset) { m_instructionPointer = offset; }

		SHARD_FORCE_INLINE InterruptionReason get_interruption() const { return m_interruption; }
		SHARD_FORCE_INLINE void set_interruption(InterruptionReason reason) { m_interruption = reason; }

		SHARD_FORCE_INLINE ObjectInstance get_current_exception() const { return m_currentException; }
		SHARD_FORCE_INLINE void set_current_exception(ObjectInstance value) { m_currentException = value; }

		SHARD_FORCE_INLINE gmt::vector<std::size_t>& get_defer_stack() { return m_deferStack; }
		SHARD_FORCE_INLINE gmt::vector<ExceptionHandlerRecord>& get_exception_handlers() { return m_exceptionHandlers; }

		SHARD_FORCE_INLINE std::uint16_t get_argument_count() const { return m_argumentCount; }
		SHARD_FORCE_INLINE std::uint16_t get_variable_count() const { return m_variableCount; }

		SHARD_FORCE_INLINE std::uint32_t get_pending_children() const { return m_pendingChildren; }
		SHARD_FORCE_INLINE void increment_pending_children() { ++m_pendingChildren; }
		SHARD_FORCE_INLINE void decrement_pending_children() { if (m_pendingChildren > 0) --m_pendingChildren; }

		SHARD_FORCE_INLINE std::byte* get_async_state() const { return m_asyncState; }
		SHARD_FORCE_INLINE void set_async_state(std::byte* state) { m_asyncState = state; }

		SHARD_FORCE_INLINE std::byte* get_awaited_state() const { return m_awaitedState; }
		SHARD_FORCE_INLINE void set_awaited_state(std::byte* state) { m_awaitedState = state; }

		SHARD_FORCE_INLINE bool get_suspended_at_return() const { return m_suspendedAtReturn; }
		SHARD_FORCE_INLINE void set_suspended_at_return(bool value) { m_suspendedAtReturn = value; }

		SHARD_FORCE_INLINE bool get_is_runnable() const { return m_runnable; }
		SHARD_FORCE_INLINE void set_is_runnable(bool value) { m_runnable = value; }

		SHARD_FORCE_INLINE void push_entry(const TypeShape* shape, bool isReferenceStorage, const void* payload, std::size_t payloadBytes)
		{
			if (m_evalDepth >= m_evalCapacity)
				throw std::overflow_error("eval stack overflow");

			std::byte* entry = entry_at(m_evalDepth);
			std::uintptr_t header = pack_header(shape, isReferenceStorage);
			std::memcpy(entry, &header, EntryHeaderSize);

			if (payloadBytes > 0)
				std::memcpy(entry + EntryHeaderSize, payload, payloadBytes);

			++m_evalDepth;
		}

		SHARD_FORCE_INLINE ObjectInstance pop_entry()
		{
			if (m_evalDepth == 0)
				throw std::underflow_error("eval stack underflow");

			--m_evalDepth;
			FrameEntryView view = entry_view(entry_at(m_evalDepth));
			if (view.isReferenceStorage)
			{
				std::byte* pointer;
				std::memcpy(&pointer, view.payloadAddress, sizeof(std::byte*));
				return ObjectInstance(view.shape, pointer, m_collector);
			}

			return ObjectInstance(view.shape, view.payloadAddress, m_collector);
		}

		SHARD_FORCE_INLINE ObjectInstance peek_entry() const
		{
			if (m_evalDepth == 0)
				throw std::underflow_error("eval stack underflow");

			FrameEntryView view = entry_view(entry_at(static_cast<std::size_t>(m_evalDepth) - 1));
			if (view.isReferenceStorage)
			{
				std::byte* pointer;
				std::memcpy(&pointer, view.payloadAddress, sizeof(std::byte*));
				return ObjectInstance(view.shape, pointer, m_collector);
			}

			return ObjectInstance(view.shape, view.payloadAddress, m_collector);
		}

		SHARD_FORCE_INLINE ObjectInstance peek_entry_at(std::size_t depthFromTop) const
		{
			if (depthFromTop >= m_evalDepth)
				throw std::underflow_error("eval stack underflow");

			FrameEntryView view = entry_view(entry_at(static_cast<std::size_t>(m_evalDepth) - 1 - depthFromTop));
			if (view.isReferenceStorage)
			{
				std::byte* pointer;
				std::memcpy(&pointer, view.payloadAddress, sizeof(std::byte*));
				return ObjectInstance(view.shape, pointer, m_collector);
			}

			return ObjectInstance(view.shape, view.payloadAddress, m_collector);
		}

		SHARD_FORCE_INLINE std::size_t get_eval_depth() const { return m_evalDepth; }
		SHARD_FORCE_INLINE void reset_eval_depth(std::size_t depth) { m_evalDepth = static_cast<std::uint32_t>(depth); }

		SHARD_FORCE_INLINE ObjectInstance get_local(std::uint16_t slot) const
		{
			return ObjectInstance(m_localSlots[slot].shape, m_variableSlots[slot], m_collector);
		}

		SHARD_FORCE_INLINE void set_local(std::uint16_t slot, ObjectInstance value)
		{
			if (slot >= m_localCount)
				throw std::runtime_error("local slot index is out of range");

			LocalSlot& local = m_localSlots[slot];
			if (value.get_shape() != nullptr && (local.shape == nullptr || local.isOpen))
				local.shape = value.get_shape();

			if (local.shape == nullptr || local.shape->is_reference())
			{
				m_variableSlots[slot] = value.get_memory();
				return;
			}

			if (value == nullptr)
			{
				m_variableSlots[slot] = nullptr;
				return;
			}

			std::byte* destination = local.dataAddress;
			if (destination == nullptr)
				destination = m_variableSlots[slot];

			if (destination == nullptr)
				destination = allocate_local_cell(slot);

			m_variableSlots[slot] = destination;
			std::memcpy(destination, value.get_memory(), local.shape->get_size());
		}

		SHARD_FORCE_INLINE void push_local(std::uint16_t slot)
		{
			const LocalSlot& local = m_localSlots[slot];
			std::byte* pointer = m_variableSlots[slot];
			if (pointer == nullptr || local.shape == nullptr || local.shape->is_reference())
			{
				push_entry(local.shape, true, &pointer, sizeof(std::byte*));
				return;
			}

			push_entry(local.shape, false, pointer, local.shape->get_size());
		}

		ObjectInstance get_local_address(std::uint16_t slot) const;
		ObjectInstance ensure_local_storage(std::uint16_t slot);
		FrameEntryView get_local_view(std::uint16_t slot) const;
		std::byte* get_local_entry(std::uint16_t slot) const;

		void complete_return(ObjectInstance value);
		void set_return_result(ObjectInstance value);
		FrameEntryView get_return_view() const;

		SHARD_FORCE_INLINE ObjectInstance get_return_slot() const { return m_returnResult; }
		SHARD_FORCE_INLINE ObjectInstance get_return_destination() const { return ObjectInstance(m_returnShape, m_returnDestination, m_collector); }
		SHARD_FORCE_INLINE std::byte* get_return_entry() const { return m_returnDestination; }
		SHARD_FORCE_INLINE std::size_t get_return_payload_bytes() const { return m_returnShape != nullptr ? m_returnShape->get_size() : 0; }
		SHARD_FORCE_INLINE bool has_external_return_slot() const { return true; }

		template<typename F>
		void for_each_live_entry(F&& visitor)
		{
			for (std::uint32_t i = 0; i < m_evalDepth; ++i)
			{
				FrameEntryView view = entry_view(entry_at(i));
				if (view.shape != nullptr)
					visitor(view);
			}
		}

		template<typename F>
		void for_each_local(F&& visitor) const
		{
			for (std::size_t i = 0; i < m_localCount; ++i)
				visitor(get_local(static_cast<std::uint16_t>(i)));
		}
	};
}
