#pragma once
#include <shard/Definitions.hpp>

#include <shard/semantic/SemanticModel.hpp>
#include <shard/semantic/SyntaxSymbol.hpp>
#include <shard/semantic/symbols/MethodSymbol.hpp>
#include <shard/semantic/symbols/StructSymbol.hpp>

#include <shard/pragmatic/PragmaticModel.hpp>

#include <shard/runtime/CallStackFrame.hpp>
#include <shard/runtime/GarbageCollector.hpp>
#include <shard/runtime/ObjectInstance.hpp>
#include <shard/runtime/PrimitiveMath.hpp>

#include <shard/compilation/OperationCode.hpp>
#include <shard/compilation/ByteCodeDecoder.hpp>
#include <shard/compilation/VirtualProgramImage.hpp>

#include <gmt/Containers.hpp>
#include <gmt/Ref.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace shard
{
	struct AsyncRuntime;
	class EventLoop;
	struct TaskRuntime;

	class SHARD_API VirtualMachine
	{
		VirtualProgramImage& m_program;
		SemanticModel& m_model;
		PragmaticModel& m_pragmatic;

		GarbageCollector m_collector;
		PrimitiveShapes m_primitiveShapes;

		// interned instance per LOADCONST_STRING literal id; intern_string
		// already deduplicates by content, this only skips the lookup
		gmt::vector<ObjectInstance> m_stringLiterals;

		bool m_hasUnhandledException = false;
		ObjectInstance m_unhandledException;
		ObjectInstance* m_exceptionCapture = nullptr;

		CallStackFrame* m_currentFrame = nullptr;
		FrameBlockPool m_framePool;
		alignas(8) std::byte m_returnScratch[8] = {};

		// async task table, runnable queue and the libuv loop; declared after
		// the collector so it is torn down before the collector disappears
		gmt::unique_ptr<AsyncRuntime> m_async;

		// Interface calls repeat the same (concrete receiver type, interface
		// method) pair in hot loops. Resolution is content-based and stable
		// after the semantic phase, so a direct-mapped cache removes the
		// per-call scan of the receiver's member lists.
		struct InterfaceDispatchEntry
		{
			StructSymbol* receiverType = nullptr;
			MethodSymbol* interfaceMethod = nullptr;
			MethodSymbol* implementation = nullptr;
		};

		static constexpr std::size_t InterfaceDispatchCacheSize = 256;
		InterfaceDispatchEntry m_interfaceDispatch[InterfaceDispatchCacheSize] = {};

		const TypeShape* resolve_shape(StructSymbol* type);
		// pointer-identity fast path for the canonical primitive shapes;
		// returns null for composites and void
		const TypeShape* primitive_shape(StructSymbol* type) const;
		const TypeShape* resolve_operand_shape(SyntaxSymbol* symbol);

		CallStackFrame* push_frame(MethodSymbol* method, const FrameLayout* layout = nullptr, std::byte* externalReturnEntry = nullptr, std::size_t externalReturnPayload = 0);
		void pop_frame();

		void execute_frame(CallStackFrame* frame);
		void process_code(CallStackFrame* frame, ByteCodeDecoder& decoder, OpCode opcode);
		bool handle_exception_in_frame(CallStackFrame* frame, ByteCodeDecoder& decoder);
		void drain_defers(CallStackFrame* frame, ByteCodeDecoder& decoder, std::size_t count);
		void run_defer_body(CallStackFrame* frame, ByteCodeDecoder& decoder, std::size_t target);

		ObjectInstance invoke_managed(MethodSymbol* method, std::span<const ObjectInstance> arguments, const FrameLayout* layout = nullptr);
		ObjectInstance invoke_native(MethodSymbol* method, std::span<const ObjectInstance> arguments);
		void invoke_into(MethodSymbol* method, std::span<const ObjectInstance> arguments, std::byte* destinationEntry, const FrameLayout* layout = nullptr);
		void write_entry_value(const FrameEntryView& view, std::size_t payloadBytes, ObjectInstance value);

		void throw_value(CallStackFrame* frame, ObjectInstance value);
		void propagate_exception(CallStackFrame* into, ObjectInstance exception);

		void push_scalar(CallStackFrame* frame, const ScalarResult& result);
		bool is_void_method(MethodSymbol* method) const;
		bool is_instance_of(const TypeShape* objectShape, StructSymbol* targetType) const;

		// async execution; task bookkeeping lives in the implementation-only
		// AsyncRuntime owned by m_async
		ObjectInstance invoke_async_method(MethodSymbol* method, std::span<const ObjectInstance> arguments, const FrameLayout* layout);
		void finish_async_frame(CallStackFrame* frame);
		void complete_task_status(ObjectInstance state, int status, ObjectInstance result, ObjectInstance exception);
		void resume_frame(CallStackFrame* frame);
		bool pump_async_once(bool block);
		void wait_for_children(CallStackFrame* frame);
		void wait_for_task(ObjectInstance state);
		void deliver_await_result(CallStackFrame* frame);
		void execute_await(CallStackFrame* frame, ByteCodeDecoder& decoder);
		TaskRuntime& ensure_task_record(ObjectInstance state);
		ObjectInstance build_runtime_exception(std::wstring_view message);
		void abandon_children(CallStackFrame* frame);
		ObjectInstance take_unobserved_fault(CallStackFrame* frame);
		void prune_tasks_for_frame(CallStackFrame* frame);
		void enqueue_runnable(CallStackFrame* frame);
		void run_pending_continuation();

	public:
		VirtualMachine(VirtualProgramImage& program, SemanticModel& model, PragmaticModel& pragmatic);
		~VirtualMachine();

		VirtualMachine(const VirtualMachine&) = delete;
		VirtualMachine& operator=(const VirtualMachine&) = delete;

		void run();
		ObjectInstance invoke_method(MethodSymbol* method, std::span<const ObjectInstance> arguments, const FrameLayout* layout = nullptr);
		ObjectInstance try_invoke_method(MethodSymbol* method, std::span<const ObjectInstance> arguments, ObjectInstance& outException);

		// async surface used by native libraries through MethodCallState
		ObjectInstance launch_async_method(MethodSymbol* method, std::span<const ObjectInstance> arguments);
		ObjectInstance await_task(ObjectInstance task);
		bool is_task_completed(ObjectInstance task);
		void complete_task(ObjectInstance task, ObjectInstance result);
		void complete_task(ObjectInstance task);
		void fail_task(ObjectInstance task, ObjectInstance exception);
		void complete_task_state(ObjectInstance state, ObjectInstance result);
		void fail_task_state(ObjectInstance state, ObjectInstance exception);
		ObjectInstance create_task_wrapper(ObjectInstance state);
		ObjectInstance delay_task(std::int64_t milliseconds);
		ObjectInstance continue_task(ObjectInstance task, ObjectInstance continuation);

		VirtualProgramImage& get_program() const;
		SemanticModel& get_model() const;
		PragmaticModel& get_pragmatic() const;
		GarbageCollector& get_collector();
		const PrimitiveShapes& get_primitive_shapes() const;

		CallStackFrame* get_current_frame() const;
		ObjectInstance create_string(std::wstring_view value);
		void raise_runtime_exception(std::wstring_view message);
		void raise_exception(ObjectInstance exception);

		bool get_has_unhandled_exception() const;
		ObjectInstance get_unhandled_exception() const;
		gmt::wstring get_unhandled_message() const;
	};
}
