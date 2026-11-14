#pragma once
#include <shard/Definitions.hpp>

#include <shard/runtime/ObjectInstance.hpp>

#include <gmt/Containers.hpp>
#include <gmt/Ref.hpp>

#include <cstddef>
#include <cstdint>
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

		static std::uintptr_t pack_header(const TypeShape* shape, bool isReferenceStorage);
		static const TypeShape* unpack_shape(std::uintptr_t header);
		static bool unpack_is_reference(std::uintptr_t header);
		FrameEntryView entry_view(std::byte* entry) const;

	public:
		static FrameEntryView peek_entry(std::byte* entry);
		static CallStackFrame* create(MethodSymbol* method, const FrameLayout* layout, SemanticModel& model, PragmaticModel& pragmatic, CallStackFrame* previous, GarbageCollector* collector = nullptr, std::byte* externalReturnDestination = nullptr, std::size_t externalReturnPayload = 0, FrameBlockPool* pool = nullptr);
		static void destroy(CallStackFrame* frame, FrameBlockPool* pool = nullptr);

		CallStackFrame(const CallStackFrame&) = delete;
		CallStackFrame& operator=(const CallStackFrame&) = delete;

		MethodSymbol* get_method() const;
		CallStackFrame* get_previous() const;

		std::size_t get_instruction_pointer() const;
		void set_instruction_pointer(std::size_t offset);

		InterruptionReason get_interruption() const;
		void set_interruption(InterruptionReason reason);

		ObjectInstance get_current_exception() const;
		void set_current_exception(ObjectInstance value);

		gmt::vector<std::size_t>& get_defer_stack();
		gmt::vector<ExceptionHandlerRecord>& get_exception_handlers();

		std::uint16_t get_argument_count() const;
		std::uint16_t get_variable_count() const;

		std::uint32_t get_pending_children() const;
		void increment_pending_children();
		void decrement_pending_children();

		std::byte* get_async_state() const;
		void set_async_state(std::byte* state);

		std::byte* get_awaited_state() const;
		void set_awaited_state(std::byte* state);

		bool get_suspended_at_return() const;
		void set_suspended_at_return(bool value);

		bool get_is_runnable() const;
		void set_is_runnable(bool value);

		void push_entry(const TypeShape* shape, bool isReferenceStorage, const void* payload, std::size_t payloadBytes);
		ObjectInstance pop_entry();
		ObjectInstance peek_entry() const;
		ObjectInstance peek_entry_at(std::size_t depthFromTop) const;
		std::size_t get_eval_depth() const;
		void reset_eval_depth(std::size_t depth);

		ObjectInstance get_local(std::uint16_t slot) const;
		void set_local(std::uint16_t slot, ObjectInstance value);
		void push_local(std::uint16_t slot);
		ObjectInstance get_local_address(std::uint16_t slot) const;
		ObjectInstance ensure_local_storage(std::uint16_t slot);
		FrameEntryView get_local_view(std::uint16_t slot) const;
		std::byte* get_local_entry(std::uint16_t slot) const;

		void complete_return(ObjectInstance value);
		void set_return_result(ObjectInstance value);
		FrameEntryView get_return_view() const;
		ObjectInstance get_return_slot() const;
		ObjectInstance get_return_destination() const;
		std::byte* get_return_entry() const;
		std::size_t get_return_payload_bytes() const;
		bool has_external_return_slot() const;

		template<typename F>
		void for_each_live_entry(F&& visitor)
		{
			for (std::uint32_t i = 0; i < m_evalDepth; ++i)
			{
				FrameEntryView view = entry_view(m_evalRegion + m_evalStride * i);
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
