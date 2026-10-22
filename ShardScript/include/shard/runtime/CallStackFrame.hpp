#pragma once
#include <shard/Definitions.hpp>

#include <shard/runtime/ObjectInstance.hpp>

#include <gmt/Containers.hpp>
#include <gmt/Ref.hpp>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>

namespace shard
{
	class MethodSymbol;
	class FrameLayout;
	class SemanticModel;
	class PragmaticModel;

	enum class InterruptionReason
	{
		None,
		ValueReturned,
		ExceptionRaised
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

	class SHARD_API CallStackFrame
	{
	public:
		static constexpr std::size_t EntryHeaderSize = sizeof(std::uintptr_t);
		static constexpr std::uintptr_t ReferenceTag = 1;

	private:
		gmt::Ref<MethodSymbol> m_method;
		CallStackFrame* m_previous = nullptr;
		std::size_t m_instructionPointer = 0;
		InterruptionReason m_interruption = InterruptionReason::None;
		ObjectInstance m_currentException;

		gmt::vector<std::size_t> m_deferStack;
		gmt::vector<ExceptionHandlerRecord> m_exceptionHandlers;

		std::uint16_t m_argumentCount = 0;
		std::uint16_t m_variableCount = 0;

		std::byte* m_returnSlot = nullptr;
		std::size_t m_returnPayloadBytes = 0;
		bool m_hasExternalReturnSlot = false;

		std::byte* m_localsRegion = nullptr;
		gmt::vector<std::byte*> m_localAddresses;
		gmt::vector<std::size_t> m_localPayloads;

		std::byte* m_evalRegion = nullptr;
		std::size_t m_evalStride = 0;
		std::size_t m_evalCapacity = 0;
		std::uint32_t m_evalDepth = 0;

		CallStackFrame() = default;

		static std::uintptr_t pack_header(const TypeShape* shape, bool isReferenceStorage);
		static const TypeShape* unpack_shape(std::uintptr_t header);
		static bool unpack_is_reference(std::uintptr_t header);

		std::byte* local_entry(std::uint16_t slot) const;
		FrameEntryView entry_view(std::byte* entry) const;

	public:
		static FrameEntryView peek_entry(std::byte* entry);
		static CallStackFrame* create(gmt::Ref<MethodSymbol> method, const FrameLayout* layout, SemanticModel& model, PragmaticModel& pragmatic, CallStackFrame* previous, std::byte* externalReturnEntry = nullptr, std::size_t externalReturnPayload = 0);
		static void destroy(CallStackFrame* frame);

		CallStackFrame(const CallStackFrame&) = delete;
		CallStackFrame& operator=(const CallStackFrame&) = delete;

		gmt::Ref<MethodSymbol> get_method() const;
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
		FrameEntryView get_local_view(std::uint16_t slot) const;
		std::byte* get_local_entry(std::uint16_t slot) const;

		void place_returned(ObjectInstance value);
		ObjectInstance get_return_slot() const;
		FrameEntryView get_return_view() const;
		std::byte* get_return_entry() const;
		std::size_t get_return_payload_bytes() const;
		bool has_external_return_slot() const;

		void for_each_live_entry(const std::function<void(const FrameEntryView&)>& visitor);
	};
}
