#pragma once
#include <shard/Definitions.hpp>

#include <shard/runtime/CallStackFrame.hpp>
#include <shard/runtime/GarbageCollector.hpp>
#include <shard/runtime/ObjectInstance.hpp>
#include <shard/runtime/PrimitiveMath.hpp>

#include <shard/compilation/OperationCode.hpp>

#include <gmt/Ref.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace shard
{
	class VirtualProgramImage;
	class SemanticModel;
	class PragmaticModel;
	class MethodSymbol;
	class StructSymbol;
	class SyntaxSymbol;
	class ByteCodeDecoder;

	class SHARD_API VirtualMachine
	{
		VirtualProgramImage& m_program;
		SemanticModel& m_model;
		PragmaticModel& m_pragmatic;
		GarbageCollector m_collector;
		PrimitiveShapes m_primitiveShapes;

		CallStackFrame* m_currentFrame = nullptr;
		ObjectInstance m_unhandledException;
		std::wstring m_unhandledMessage;
		bool m_hasUnhandledException = false;
		alignas(8) std::byte m_returnScratch[8] = {};

		const TypeShape* resolve_shape(gmt::Ref<StructSymbol> type);
		const TypeShape* resolve_operand_shape(gmt::Ref<SyntaxSymbol> symbol);

		CallStackFrame* push_frame(gmt::Ref<MethodSymbol> method);
		void pop_frame();

		void execute_frame(CallStackFrame* frame);
		void process_code(CallStackFrame* frame, ByteCodeDecoder& decoder, OpCode opcode);
		bool handle_exception_in_frame(CallStackFrame* frame, ByteCodeDecoder& decoder);
		void drain_defers(CallStackFrame* frame, ByteCodeDecoder& decoder, std::size_t count);
		void run_defer_body(CallStackFrame* frame, ByteCodeDecoder& decoder, std::size_t target);

		ObjectInstance invoke_managed(gmt::Ref<MethodSymbol> method, std::span<const ObjectInstance> arguments);
		ObjectInstance invoke_native(gmt::Ref<MethodSymbol> method, std::span<const ObjectInstance> arguments);

		void throw_value(CallStackFrame* frame, ObjectInstance value);
		void propagate_exception(CallStackFrame* into, ObjectInstance exception);

		void push_scalar(CallStackFrame* frame, const ScalarResult& result);
		bool is_void_method(gmt::Ref<MethodSymbol> method) const;
		bool is_instance_of(const TypeShape* objectShape, gmt::Ref<StructSymbol> targetType) const;

	public:
		VirtualMachine(VirtualProgramImage& program, SemanticModel& model, PragmaticModel& pragmatic);
		~VirtualMachine();

		VirtualMachine(const VirtualMachine&) = delete;
		VirtualMachine& operator=(const VirtualMachine&) = delete;

		void run();
		ObjectInstance invoke_method(gmt::Ref<MethodSymbol> method, std::span<const ObjectInstance> arguments);

		VirtualProgramImage& get_program() const;
		SemanticModel& get_model() const;
		PragmaticModel& get_pragmatic() const;
		GarbageCollector& get_collector();
		const PrimitiveShapes& get_primitive_shapes() const;

		CallStackFrame* get_current_frame() const;
		ObjectInstance create_string(std::wstring_view value);
		void raise_runtime_exception(std::wstring_view message);

		bool get_has_unhandled_exception() const;
		ObjectInstance get_unhandled_exception() const;
		const std::wstring& get_unhandled_message() const;
	};
}
