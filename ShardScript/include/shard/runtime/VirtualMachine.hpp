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
	class SHARD_API VirtualMachine
	{
		VirtualProgramImage& m_program;
		SemanticModel& m_model;
		PragmaticModel& m_pragmatic;

		GarbageCollector m_collector;
		PrimitiveShapes m_primitiveShapes;

		bool m_hasUnhandledException = false;
		ObjectInstance m_unhandledException;
		ObjectInstance* m_exceptionCapture = nullptr;

		CallStackFrame* m_currentFrame = nullptr;
		FrameBlockPool m_framePool;
		alignas(8) std::byte m_returnScratch[8] = {};

		const TypeShape* resolve_shape(StructSymbol* type);
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

	public:
		VirtualMachine(VirtualProgramImage& program, SemanticModel& model, PragmaticModel& pragmatic);
		~VirtualMachine();

		VirtualMachine(const VirtualMachine&) = delete;
		VirtualMachine& operator=(const VirtualMachine&) = delete;

		void run();
		ObjectInstance invoke_method(MethodSymbol* method, std::span<const ObjectInstance> arguments, const FrameLayout* layout = nullptr);
		ObjectInstance try_invoke_method(MethodSymbol* method, std::span<const ObjectInstance> arguments, ObjectInstance& outException);

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
