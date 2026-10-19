#include <shard/LibraryMetadata.hpp>

#include <shard/runtime/VirtualMachine.hpp>
#include <shard/runtime/MethodCallState.hpp>

#include <shard/compilation/ByteCodeDecoder.hpp>
#include <shard/compilation/VirtualProgramImage.hpp>

#include <shard/parsing/SyntaxKind.hpp>

#include <shard/pragmatic/FrameLayout.hpp>
#include <shard/pragmatic/PragmaticModel.hpp>
#include <shard/pragmatic/TypeShape.hpp>

#include <shard/semantic/SemanticModel.hpp>
#include <shard/semantic/SymbolTable.hpp>
#include <shard/semantic/symbols/AccessorSymbol.hpp>
#include <shard/semantic/symbols/ArrayTypeSymbol.hpp>
#include <shard/semantic/symbols/DelegateTypeSymbol.hpp>
#include <shard/semantic/symbols/FieldSymbol.hpp>
#include <shard/semantic/symbols/GenericTypeSymbol.hpp>
#include <shard/semantic/symbols/MethodSymbol.hpp>
#include <shard/semantic/symbols/NullableTypeSymbol.hpp>
#include <shard/semantic/symbols/PropertySymbol.hpp>
#include <shard/semantic/symbols/StructSymbol.hpp>

#include <gmt/Concat.hpp>
#include <gmt/InlineVector.hpp>
#include <gmt/Strings.hpp>

#include <cstring>
#include <stdexcept>

using namespace shard;

namespace
{
	std::size_t native_argument_count(gmt::Ref<MethodSymbol> method)
	{
		std::size_t count = method->get_parameters().size();
		if (method->get_linking() == LINK_INSTANCE)
			++count;

		return count;
	}

	void native_array_length(MethodCallState& state)
	{
		state.set_return_integer(state.get_argument(0).as_integer());
	}

	void native_range_move_next(MethodCallState& state)
	{
		ObjectInstance self = state.get_argument(0);
		SymbolTable& table = state.get_vm().get_model().get_table();

		gmt::Ref<FieldSymbol> currentField = table.get_standard_type(StandardTypes::RangeEnumerable_CurrentField).as<FieldSymbol>();
		gmt::Ref<FieldSymbol> toField = table.get_standard_type(StandardTypes::RangeEnumerable_ToField).as<FieldSymbol>();

		ObjectInstance current = self.get_field(currentField);
		std::int64_t value = current.as_integer() + 1;
		current.write_integer(value);

		state.set_return_boolean(value < self.get_field(toField).as_integer());
	}

	void native_range_current_get(MethodCallState& state)
	{
		SymbolTable& table = state.get_vm().get_model().get_table();
		gmt::Ref<FieldSymbol> currentField = table.get_standard_type(StandardTypes::RangeEnumerable_CurrentField).as<FieldSymbol>();

		state.set_return_integer(state.get_argument(0).get_field(currentField).as_integer());
	}
}

VirtualMachine::VirtualMachine(VirtualProgramImage& program, SemanticModel& model, PragmaticModel& pragmatic)
	: m_program(program)
	, m_model(model)
	, m_pragmatic(pragmatic)
	, m_collector(model, pragmatic)
{
	SymbolTable& table = m_model.get_table();
	m_primitiveShapes.boolean = resolve_shape(table.get_boolean_primitive());
	m_primitiveShapes.integer = resolve_shape(table.get_integer_primitive());
	m_primitiveShapes.doubleReal = resolve_shape(table.get_double_primitive());
	m_primitiveShapes.character = resolve_shape(table.get_char_primitive());
	m_primitiveShapes.nint = resolve_shape(table.get_nint_primitive());
	m_primitiveShapes.byte = resolve_shape(table.get_byte_primitive());
	m_primitiveShapes.string = resolve_shape(table.get_string_primitive());

	for (gmt::Ref<PropertySymbol> property : table.get_array_primitive()->properties)
	{
		if (property.is_null() || property->get_name() != L"Length")
			continue;

		if (!property->get_getter().is_null())
			property->get_getter()->set_function_pointer(reinterpret_cast<void*>(&native_array_length));
	}

	gmt::Ref<SyntaxSymbol> rangeMoveNext = table.get_standard_type(StandardTypes::RangeEnumerable_MoveNext);
	if (!rangeMoveNext.is_null() && rangeMoveNext->is_method())
		rangeMoveNext.as<MethodSymbol>()->set_function_pointer(reinterpret_cast<void*>(&native_range_move_next));

	gmt::Ref<SyntaxSymbol> rangeCurrentGet = table.get_standard_type(StandardTypes::RangeEnumerable_Current_get);
	if (!rangeCurrentGet.is_null() && rangeCurrentGet->is_method())
		rangeCurrentGet.as<MethodSymbol>()->set_function_pointer(reinterpret_cast<void*>(&native_range_current_get));
}

VirtualMachine::~VirtualMachine()
{
	while (m_currentFrame != nullptr)
		pop_frame();
}

const TypeShape* VirtualMachine::resolve_shape(gmt::Ref<StructSymbol> type)
{
	if (type.is_null())
		return nullptr;

	return m_pragmatic.find_or_create_type_shape(m_model, type);
}

const TypeShape* VirtualMachine::resolve_operand_shape(gmt::Ref<SyntaxSymbol> symbol)
{
	if (symbol.is_null() || !symbol->is_type())
		return nullptr;

	return resolve_shape(symbol.as<StructSymbol>());
}

CallStackFrame* VirtualMachine::push_frame(gmt::Ref<MethodSymbol> method, std::byte* externalReturnEntry, std::size_t externalReturnPayload)
{
	FrameLayout* layout = m_pragmatic.find_frame_layout(method);
	CallStackFrame* frame = CallStackFrame::create(method, layout, m_model, m_pragmatic, m_currentFrame, externalReturnEntry, externalReturnPayload);
	m_collector.register_frame(frame);
	m_currentFrame = frame;
	return frame;
}

void VirtualMachine::pop_frame()
{
	CallStackFrame* frame = m_currentFrame;
	m_currentFrame = frame->get_previous();
	m_collector.unregister_frame(frame);
	CallStackFrame::destroy(frame);
}

bool VirtualMachine::is_void_method(gmt::Ref<MethodSymbol> method) const
{
	if (method->get_return_type().is_null())
		return true;

	return method->get_return_type() == m_model.get_table().get_void_primitive();
}

bool VirtualMachine::is_instance_of(const TypeShape* objectShape, gmt::Ref<StructSymbol> targetType) const
{
	if (objectShape == nullptr || targetType.is_null())
		return false;

	gmt::Ref<StructSymbol> baseType = objectShape->get_base_type();
	while (baseType->get_kind() == SyntaxKind::NullableType)
		baseType = baseType.as<NullableTypeSymbol>()->get_underlaying_type();

	while (targetType->get_kind() == SyntaxKind::NullableType)
		targetType = targetType.as<NullableTypeSymbol>()->get_underlaying_type();

	if (baseType == targetType)
		return true;

	for (gmt::Ref<StructSymbol> interfaceType : baseType->interfaces)
	{
		if (interfaceType.is_null())
			continue;

		gmt::Ref<StructSymbol> candidate = interfaceType;
		if (candidate->get_kind() == SyntaxKind::GenericType)
			candidate = candidate.as<GenericTypeSymbol>()->get_underlaying_type();

		if (candidate == targetType)
			return true;
	}

	return false;
}

void VirtualMachine::push_scalar(CallStackFrame* frame, const ScalarResult& result)
{
	std::size_t bytes = result.shape != nullptr ? result.shape->get_size() : sizeof(std::uint64_t);
	frame->push_entry(result.shape, false, &result.payload, bytes);
}

void VirtualMachine::propagate_exception(CallStackFrame* into, ObjectInstance exception)
{
	into->set_current_exception(exception);
	into->set_interruption(InterruptionReason::ExceptionRaised);
}

void VirtualMachine::throw_value(CallStackFrame* frame, ObjectInstance value)
{
	ObjectInstance payload = value;
	const TypeShape* shape = value.get_shape();

	if (shape != nullptr && !shape->is_reference())
	{
		// primitives are boxed so in-flight exceptions are uniformly heap objects (E7)
		std::uint64_t scratch = 0;
		std::memcpy(&scratch, value.get_memory(), shape->get_size());

		payload = m_collector.allocate_instance(shape);
		std::memcpy(payload.get_memory(), &scratch, shape->get_size());
	}

	propagate_exception(frame, payload);
}

void VirtualMachine::raise_runtime_exception(std::wstring_view message)
{
	ObjectInstance exception = create_string(message);
	if (m_currentFrame == nullptr)
	{
		m_hasUnhandledException = true;
		m_unhandledException = exception;
		m_unhandledMessage = std::wstring(message);
		return;
	}

	propagate_exception(m_currentFrame, exception);
}

ObjectInstance VirtualMachine::create_string(std::wstring_view value)
{
	return m_collector.allocate_string(value);
}

void VirtualMachine::execute_frame(CallStackFrame* frame)
{
	gmt::Ref<MethodSymbol> method = frame->get_method();
	std::size_t start = reinterpret_cast<std::size_t>(method->get_function_pointer());
	ByteCodeDecoder decoder(m_program, start);

	while (true)
	{
		InterruptionReason interruption = frame->get_interruption();
		if (interruption == InterruptionReason::ExceptionRaised)
		{
			if (handle_exception_in_frame(frame, decoder))
				continue;

			return;
		}

		if (interruption == InterruptionReason::ValueReturned)
			return;

		if (decoder.is_eof())
			return;

		OpCode opcode = decoder.absorb_opcode();
		process_code(frame, decoder, opcode);
	}
}

void VirtualMachine::write_entry_value(const FrameEntryView& view, std::size_t payloadBytes, ObjectInstance value)
{
	if (view.payloadAddress == value.get_memory())
		return;

	if (!view.isReferenceStorage)
	{
		std::memcpy(view.payloadAddress, value.get_memory(), payloadBytes);
		return;
	}

	// reference-storage entries pass heap instances through by pointer, but a
	// frame-local payload would dangle once its frame dies: box it first
	bool valueIsComposite = value.get_shape() != nullptr && value.get_shape()->is_reference();
	if (valueIsComposite && !m_collector.is_heap_backed(value.get_memory()))
	{
		ObjectInstance boxed = m_collector.allocate_instance(value.get_shape());
		std::memcpy(boxed.get_memory(), value.get_memory(), value.get_shape()->get_size());
		value = boxed;
	}

	std::byte* pointer = value.get_memory();
	std::memcpy(view.payloadAddress, &pointer, sizeof(std::byte*));
}

void VirtualMachine::invoke_into(gmt::Ref<MethodSymbol> method, std::span<const ObjectInstance> arguments, std::byte* destinationEntry)
{
	const TypeShape* returnShape = resolve_shape(method->get_return_type());
	std::size_t returnPayload = returnShape != nullptr ? returnShape->get_size() : 0;

	if (method->get_calling_convention() == CallingConvention::ShardDecl)
	{
		ObjectInstance result = invoke_native(method, arguments);
		FrameEntryView view = CallStackFrame::peek_entry(destinationEntry);
		write_entry_value(view, returnPayload, result);
		return;
	}

	CallStackFrame* frame = push_frame(method, destinationEntry, returnPayload);
	for (std::size_t i = 0; i < arguments.size(); ++i)
		frame->set_local(static_cast<std::uint16_t>(i), arguments[i]);

	execute_frame(frame);

	bool raised = frame->get_interruption() == InterruptionReason::ExceptionRaised;
	ObjectInstance exception = frame->get_current_exception();
	pop_frame();

	if (!raised)
		return;

	if (m_currentFrame == nullptr)
	{
		m_hasUnhandledException = true;
		m_unhandledException = exception;
		m_unhandledMessage = PrimitiveMath::to_display_string(exception);
		return;
	}

	propagate_exception(m_currentFrame, exception);
}

ObjectInstance VirtualMachine::invoke_managed(gmt::Ref<MethodSymbol> method, std::span<const ObjectInstance> arguments)
{
	CallStackFrame* frame = push_frame(method);
	for (std::size_t i = 0; i < arguments.size(); ++i)
		frame->set_local(static_cast<std::uint16_t>(i), arguments[i]);

	execute_frame(frame);

	bool raised = frame->get_interruption() == InterruptionReason::ExceptionRaised;
	ObjectInstance exception = frame->get_current_exception();

	ObjectInstance result = ObjectInstance::null();
	if (!raised && !is_void_method(method))
	{
		ObjectInstance view = frame->get_return_slot();
		const TypeShape* returnShape = view.get_shape();

		bool inlineValue = returnShape != nullptr && !returnShape->is_reference();
		if (inlineValue)
		{
			std::memcpy(m_returnScratch, view.get_memory(), returnShape->get_size());
			result = ObjectInstance(returnShape, m_returnScratch);
		}
		else
		{
			result = view;
		}
	}

	pop_frame();

	if (raised)
	{
		if (m_currentFrame == nullptr)
		{
			m_hasUnhandledException = true;
			m_unhandledException = exception;
			m_unhandledMessage = PrimitiveMath::to_display_string(exception);
			return ObjectInstance::null();
		}

		propagate_exception(m_currentFrame, exception);
		return ObjectInstance::null();
	}

	return result;
}

ObjectInstance VirtualMachine::invoke_native(gmt::Ref<MethodSymbol> method, std::span<const ObjectInstance> arguments)
{
	for (const ObjectInstance& argument : arguments)
		m_collector.pin(argument);

	MethodSymbolDelegate callback = reinterpret_cast<MethodSymbolDelegate>(method->get_function_pointer());
	MethodCallState state(*this, m_currentFrame, arguments);

	try
	{
		callback(state);
	}
	catch (const std::exception& error)
	{
		m_collector.unpin(arguments.size());
		raise_runtime_exception(gmt::utf8_to_wide(error.what()));
		return ObjectInstance::null();
	}

	m_collector.unpin(arguments.size());

	ObjectInstance result = state.get_return_value();
	const TypeShape* returnShape = result.get_shape();
	if (returnShape != nullptr && !returnShape->is_reference() && !result.is_null())
	{
		std::memcpy(m_returnScratch, result.get_memory(), returnShape->get_size());
		result = ObjectInstance(returnShape, m_returnScratch);
	}

	return result;
}

ObjectInstance VirtualMachine::invoke_method(gmt::Ref<MethodSymbol> method, std::span<const ObjectInstance> arguments)
{
	CallingConvention convention = method->get_calling_convention();
	switch (convention)
	{
		case CallingConvention::Managed:
			return invoke_managed(method, arguments);

		case CallingConvention::ShardDecl:
			return invoke_native(method, arguments);

		default:
			break;
	}

	throw std::runtime_error("Method calling convention was 'None'");
}

bool VirtualMachine::handle_exception_in_frame(CallStackFrame* frame, ByteCodeDecoder& decoder)
{
	std::vector<ExceptionHandlerRecord>& handlers = frame->get_exception_handlers();
	if (handlers.empty())
	{
		drain_defers(frame, decoder, frame->get_defer_stack().size());
		return false;
	}

	ExceptionHandlerRecord record = handlers.back();
	handlers.pop_back();

	std::size_t pendingDefers = frame->get_defer_stack().size() - record.deferStackBase;
	drain_defers(frame, decoder, pendingDefers);

	frame->reset_eval_depth(record.evalDepthBase);

	ObjectInstance exception = frame->get_current_exception();
	const TypeShape* shape = exception.get_shape();
	std::byte* pointer = exception.get_memory();
	frame->push_entry(shape, true, &pointer, sizeof(std::byte*));

	decoder.set_cursor(static_cast<std::int64_t>(record.handlerOffset));
	frame->set_interruption(InterruptionReason::None);
	return true;
}

void VirtualMachine::drain_defers(CallStackFrame* frame, ByteCodeDecoder& decoder, std::size_t count)
{
	// the frame may be mid-unwind; clear the flag so defer bodies can run,
	// and restore it afterwards unless a body raised a new exception
	InterruptionReason pending = frame->get_interruption();
	frame->set_interruption(InterruptionReason::None);

	std::vector<std::size_t>& defers = frame->get_defer_stack();
	for (std::size_t i = 0; i < count && !defers.empty(); ++i)
	{
		std::size_t target = defers.back();
		defers.pop_back();

		run_defer_body(frame, decoder, target);

		if (frame->get_interruption() == InterruptionReason::ExceptionRaised)
			return;
	}

	if (pending != InterruptionReason::None)
		frame->set_interruption(pending);
}

void VirtualMachine::run_defer_body(CallStackFrame* frame, ByteCodeDecoder& decoder, std::size_t target)
{
	std::size_t resume = decoder.get_cursor();
	decoder.set_cursor(static_cast<std::int64_t>(target));

	while (true)
	{
		if (decoder.is_eof())
			break;

		OpCode opcode = decoder.absorb_opcode();
		if (opcode == OpCode::DEFER_BREAK)
			break;

		process_code(frame, decoder, opcode);

		if (frame->get_interruption() == InterruptionReason::ExceptionRaised)
			break;
	}

	decoder.set_cursor(static_cast<std::int64_t>(resume));
}

void VirtualMachine::run()
{
	gmt::Ref<MethodSymbol> entryPoint = m_program.get_entry_point();
	if (entryPoint.is_null())
		throw std::runtime_error("program image has no entry point");

	invoke_method(entryPoint, {});
}

VirtualProgramImage& VirtualMachine::get_program() const
{
	return m_program;
}

SemanticModel& VirtualMachine::get_model() const
{
	return m_model;
}

PragmaticModel& VirtualMachine::get_pragmatic() const
{
	return m_pragmatic;
}

GarbageCollector& VirtualMachine::get_collector()
{
	return m_collector;
}

const PrimitiveShapes& VirtualMachine::get_primitive_shapes() const
{
	return m_primitiveShapes;
}

CallStackFrame* VirtualMachine::get_current_frame() const
{
	return m_currentFrame;
}

bool VirtualMachine::get_has_unhandled_exception() const
{
	return m_hasUnhandledException;
}

ObjectInstance VirtualMachine::get_unhandled_exception() const
{
	return m_unhandledException;
}

const std::wstring& VirtualMachine::get_unhandled_message() const
{
	return m_unhandledMessage;
}

namespace
{
	void push_instance(CallStackFrame* frame, ObjectInstance instance)
	{
		const TypeShape* shape = instance.get_shape();
		if (shape == nullptr)
		{
			std::byte* pointer = nullptr;
			frame->push_entry(nullptr, true, &pointer, sizeof(std::byte*));
			return;
		}

		if (shape->is_reference())
		{
			std::byte* pointer = instance.get_memory();
			frame->push_entry(shape, true, &pointer, sizeof(std::byte*));
			return;
		}

		frame->push_entry(shape, false, instance.get_memory(), shape->get_size());
	}
}

void VirtualMachine::process_code(CallStackFrame* frame, ByteCodeDecoder& decoder, OpCode opcode)
{
	switch (opcode)
	{
		case OpCode::NOP:
			break;

		case OpCode::HALT:
		{
			frame->set_interruption(InterruptionReason::ValueReturned);
			break;
		}

		case OpCode::POP:
		{
			frame->pop_entry();
			break;
		}

		case OpCode::LOADCONST_NULL:
		{
			std::byte* pointer = nullptr;
			frame->push_entry(nullptr, true, &pointer, sizeof(std::byte*));
			break;
		}

		case OpCode::LOADCONST_BOOLEAN:
		{
			bool value = decoder.absorb_boolean();
			frame->push_entry(m_primitiveShapes.boolean, false, &value, sizeof(bool));
			break;
		}

		case OpCode::LOADCONST_INTEGER8:
		{
			std::int64_t value = decoder.absorb_uint8();
			frame->push_entry(m_primitiveShapes.integer, false, &value, sizeof(std::int64_t));
			break;
		}

		case OpCode::LOADCONST_INTEGER64:
		{
			std::int64_t value = decoder.absorb_int64();
			frame->push_entry(m_primitiveShapes.integer, false, &value, sizeof(std::int64_t));
			break;
		}

		case OpCode::LOADCONST_NATIVE_INTEGER:
		{
			std::intptr_t value = decoder.absorb_int_ptr();
			frame->push_entry(m_primitiveShapes.nint, false, &value, sizeof(std::intptr_t));
			break;
		}

		case OpCode::LOADCONST_RATIONAL64:
		{
			double value = decoder.absorb_double64();
			frame->push_entry(m_primitiveShapes.doubleReal, false, &value, sizeof(double));
			break;
		}

		case OpCode::LOADCONST_CHAR:
		{
			wchar_t value = decoder.absorb_char16();
			frame->push_entry(m_primitiveShapes.character, false, &value, sizeof(wchar_t));
			break;
		}

		case OpCode::LOADCONST_STRING:
		{
			std::size_t offset = decoder.absorb_string();
			const wchar_t* text = reinterpret_cast<const wchar_t*>(m_program.get_data_section().data() + offset);
			ObjectInstance instance = m_collector.intern_string(text);
			push_instance(frame, instance);
			break;
		}

		case OpCode::DUP:
		{
			push_instance(frame, frame->peek_entry());
			break;
		}

		case OpCode::STORE_LOCAL:
		case OpCode::STORE_ARG:
		{
			std::uint16_t slot = decoder.absorb_variable_slot();
			frame->set_local(slot, frame->pop_entry());
			break;
		}

		case OpCode::LOAD_LOCAL:
		case OpCode::LOAD_ARG:
		{
			std::uint16_t slot = decoder.absorb_variable_slot();
			frame->push_local(slot);
			break;
		}

		case OpCode::LOAD_SLOT_ADDR:
		{
			std::uint16_t slot = decoder.absorb_variable_slot();
			ObjectInstance address = frame->get_local_address(slot);
			push_instance(frame, address);
			break;
		}

		case OpCode::JUMP:
		{
			decoder.set_cursor(static_cast<std::int64_t>(decoder.absorb_jump()));
			break;
		}

		case OpCode::JUMP_TRUE:
		case OpCode::JUMP_FALSE:
		{
			std::size_t target = decoder.absorb_jump();
			bool condition = frame->pop_entry().as_boolean();

			bool take = opcode == OpCode::JUMP_TRUE ? condition : !condition;
			if (take)
				decoder.set_cursor(static_cast<std::int64_t>(target));

			break;
		}

		case OpCode::BR_NULL:
		{
			std::size_t target = decoder.absorb_jump();
			if (frame->pop_entry().is_null())
				decoder.set_cursor(static_cast<std::int64_t>(target));

			break;
		}

		case OpCode::JUMP_TABLE:
		{
			std::uint32_t count = decoder.absorb_uint32();
			gmt::inline_vector<std::size_t, 16> targets;
			for (std::uint32_t i = 0; i < count; ++i)
				targets.push_back(decoder.absorb_jump());

			std::int64_t index = frame->pop_entry().as_integer();
			if (index >= 0 && index < static_cast<std::int64_t>(count))
				decoder.set_cursor(static_cast<std::int64_t>(targets.get_vector()[static_cast<std::size_t>(index)]));

			break;
		}

		case OpCode::RETURN:
		{
			if (!is_void_method(frame->get_method()))
			{
				ObjectInstance value = frame->pop_entry();
				write_entry_value(frame->get_return_view(), frame->get_return_payload_bytes(), value);
			}

			frame->set_interruption(InterruptionReason::ValueReturned);
			break;
		}

		case OpCode::RETURN_PLACED:
		{
			frame->set_interruption(InterruptionReason::ValueReturned);
			break;
		}

		case OpCode::THROW:
		{
			throw_value(frame, frame->pop_entry());
			break;
		}

		case OpCode::RETHROW:
		{
			frame->set_interruption(InterruptionReason::ExceptionRaised);
			break;
		}

		case OpCode::MATH_ADDITION:
		case OpCode::MATH_SUBTRACT:
		case OpCode::MATH_MULTIPLY:
		case OpCode::MATH_DIVISION:
		case OpCode::MATH_MODULO:
		case OpCode::MATH_POWER:
		case OpCode::MATH_SHL:
		case OpCode::MATH_SHR:
		{
			ObjectInstance right = frame->pop_entry();
			ObjectInstance left = frame->pop_entry();

			ScalarResult result;
			PrimitiveStatus status = PrimitiveMath::binary_math(opcode, m_primitiveShapes, left, right, result);
			if (status == PrimitiveStatus::DivideByZero)
			{
				raise_runtime_exception(L"DivideByZeroException");
				break;
			}

			if (status == PrimitiveStatus::NotPrimitive)
			{
				bool leftString = left.get_shape() == m_primitiveShapes.string;
				bool rightString = right.get_shape() == m_primitiveShapes.string;

				if (opcode == OpCode::MATH_ADDITION && (leftString || rightString))
				{
					std::wstring text = gmt::concat(PrimitiveMath::to_display_string(left), PrimitiveMath::to_display_string(right));
					ObjectInstance combined = create_string(text);
					push_instance(frame, combined);
					break;
				}

				raise_runtime_exception(L"unsupported operands for arithmetic operation");
				break;
			}

			push_scalar(frame, result);
			break;
		}

		case OpCode::MATH_NEGATIVE:
		{
			ObjectInstance value = frame->pop_entry();

			ScalarResult result;
			PrimitiveStatus status = PrimitiveMath::negate(m_primitiveShapes, value, result);
			if (status != PrimitiveStatus::Success)
			{
				raise_runtime_exception(L"unsupported operand for negation");
				break;
			}

			push_scalar(frame, result);
			break;
		}

		case OpCode::COMPARE_EQUAL:
		case OpCode::COMPARE_NOTEQUAL:
		case OpCode::COMPARE_GREATER:
		case OpCode::COMPARE_GREATER_OR_EQUAL:
		case OpCode::COMPARE_LESS:
		case OpCode::COMPARE_LESS_OR_EQUAL:
		{
			ObjectInstance right = frame->pop_entry();
			ObjectInstance left = frame->pop_entry();

			ScalarResult result;
			PrimitiveStatus status = PrimitiveMath::compare(opcode, m_primitiveShapes, left, right, result);
			if (status != PrimitiveStatus::Success)
			{
				raise_runtime_exception(L"unsupported operands for comparison");
				break;
			}

			push_scalar(frame, result);
			break;
		}

		case OpCode::LOGICAL_NOT:
		{
			bool value = frame->pop_entry().as_boolean();
			bool outcome = !value;
			frame->push_entry(m_primitiveShapes.boolean, false, &outcome, sizeof(bool));
			break;
		}

		case OpCode::LOGICAL_OR:
		{
			bool right = frame->pop_entry().as_boolean();
			bool left = frame->pop_entry().as_boolean();
			bool outcome = left || right;
			frame->push_entry(m_primitiveShapes.boolean, false, &outcome, sizeof(bool));
			break;
		}

		case OpCode::LOGICAL_AND:
		{
			bool right = frame->pop_entry().as_boolean();
			bool left = frame->pop_entry().as_boolean();
			bool outcome = left && right;
			frame->push_entry(m_primitiveShapes.boolean, false, &outcome, sizeof(bool));
			break;
		}

		case OpCode::ALLOC_HEAP:
		{
			gmt::Ref<StructSymbol> type = decoder.absorb_type_symbol();
			const TypeShape* shape = resolve_shape(type);
			ObjectInstance instance = m_collector.allocate_instance(shape);
			push_instance(frame, instance);
			break;
		}

		case OpCode::LOADFIELD:
		{
			std::uint32_t slot = decoder.absorb_field_slot();
			ObjectInstance instance = frame->pop_entry();

			if (instance.is_null())
			{
				raise_runtime_exception(L"NullReferenceException");
				break;
			}

			ObjectInstance field = instance.get_field(slot);
			const TypeShape* fieldShape = field.get_shape();

			if (fieldShape->is_reference())
			{
				std::byte* pointer = field.as_pointer();
				frame->push_entry(fieldShape, true, &pointer, sizeof(std::byte*));
				break;
			}

			frame->push_entry(fieldShape, false, field.get_memory(), fieldShape->get_size());
			break;
		}

		case OpCode::STOREFIELD:
		{
			std::uint32_t slot = decoder.absorb_field_slot();
			ObjectInstance value = frame->pop_entry();
			ObjectInstance instance = frame->pop_entry();

			if (instance.is_null())
			{
				raise_runtime_exception(L"NullReferenceException");
				break;
			}

			instance.set_field(slot, value);
			break;
		}

		case OpCode::NEWARRAY:
		{
			gmt::Ref<ArrayTypeSymbol> arrayType = decoder.absorb_array_symbol();
			const TypeShape* arrayShape = resolve_shape(arrayType);
			const TypeShape* elementShape = resolve_shape(arrayType->get_underlaying_type());

			std::int64_t count = static_cast<std::int64_t>(arrayType->get_length());
			ObjectInstance instance = m_collector.allocate_array(arrayShape, elementShape, count);

			std::size_t elementStride = elementShape->is_reference() ? sizeof(void*) : elementShape->get_size();
			std::byte* elements = instance.get_memory() + sizeof(std::int64_t);

			for (std::int64_t i = count - 1; i >= 0; --i)
			{
				ObjectInstance element = frame->pop_entry();
				std::byte* target = elements + static_cast<std::size_t>(i) * elementStride;

				if (elementShape->is_reference())
				{
					std::byte* pointer = element.get_memory();
					std::memcpy(target, &pointer, sizeof(std::byte*));
					continue;
				}

				std::memcpy(target, element.get_memory(), elementStride);
			}

			push_instance(frame, instance);
			break;
		}

		case OpCode::NEWARRAY_DYNAMIC:
		{
			gmt::Ref<ArrayTypeSymbol> arrayType = decoder.absorb_array_symbol();
			const TypeShape* arrayShape = resolve_shape(arrayType);
			const TypeShape* elementShape = resolve_shape(arrayType->get_underlaying_type());

			std::int64_t count = frame->pop_entry().as_integer();
			ObjectInstance instance = m_collector.allocate_array(arrayShape, elementShape, count);
			push_instance(frame, instance);
			break;
		}

		case OpCode::LOADARRAYELEMENT:
		{
			std::int64_t index = frame->pop_entry().as_integer();
			ObjectInstance instance = frame->pop_entry();

			if (instance.is_null())
			{
				raise_runtime_exception(L"NullReferenceException");
				break;
			}

			std::int64_t length = instance.as_integer();
			if (index < 0 || index >= length)
			{
				raise_runtime_exception(L"IndexOutOfRangeException");
				break;
			}

			gmt::Ref<StructSymbol> baseType = instance.get_shape()->get_base_type();
			const TypeShape* elementShape = resolve_shape(baseType.as<ArrayTypeSymbol>()->get_underlaying_type());

			std::size_t elementStride = elementShape->is_reference() ? sizeof(void*) : elementShape->get_size();
			std::byte* elementAddress = instance.get_memory() + sizeof(std::int64_t) + static_cast<std::size_t>(index) * elementStride;

			if (elementShape->is_reference())
			{
				std::byte* pointer;
				std::memcpy(&pointer, elementAddress, sizeof(std::byte*));
				frame->push_entry(elementShape, true, &pointer, sizeof(std::byte*));
				break;
			}

			frame->push_entry(elementShape, false, elementAddress, elementShape->get_size());
			break;
		}

		case OpCode::STOREARRAYELEMENT:
		{
			ObjectInstance value = frame->pop_entry();
			std::int64_t index = frame->pop_entry().as_integer();
			ObjectInstance instance = frame->pop_entry();

			if (instance.is_null())
			{
				raise_runtime_exception(L"NullReferenceException");
				break;
			}

			std::int64_t length = instance.as_integer();
			if (index < 0 || index >= length)
			{
				raise_runtime_exception(L"IndexOutOfRangeException");
				break;
			}

			gmt::Ref<StructSymbol> baseType = instance.get_shape()->get_base_type();
			const TypeShape* elementShape = resolve_shape(baseType.as<ArrayTypeSymbol>()->get_underlaying_type());

			std::size_t elementStride = elementShape->is_reference() ? sizeof(void*) : elementShape->get_size();
			std::byte* elementAddress = instance.get_memory() + sizeof(std::int64_t) + static_cast<std::size_t>(index) * elementStride;

			if (elementShape->is_reference())
			{
				std::byte* pointer = value.get_memory();
				std::memcpy(elementAddress, &pointer, sizeof(std::byte*));
				break;
			}

			std::memcpy(elementAddress, value.get_memory(), elementStride);
			break;
		}

		case OpCode::ARRAYLENGTH:
		{
			ObjectInstance instance = frame->pop_entry();
			if (instance.is_null())
			{
				raise_runtime_exception(L"NullReferenceException");
				break;
			}

			std::int64_t length = instance.as_integer();
			frame->push_entry(m_primitiveShapes.integer, false, &length, sizeof(std::int64_t));
			break;
		}

		case OpCode::CREATERANGE:
		{
			gmt::Ref<StructSymbol> rangeType = decoder.absorb_type_symbol();
			const TypeShape* rangeShape = resolve_shape(rangeType);

			bool inclusive = frame->pop_entry().as_boolean();
			std::int64_t upper = frame->pop_entry().as_integer();
			std::int64_t lower = frame->pop_entry().as_integer();

			SymbolTable& table = m_model.get_table();
			gmt::Ref<FieldSymbol> fromField = table.get_standard_type(StandardTypes::RangeEnumerable_FromField).as<FieldSymbol>();
			gmt::Ref<FieldSymbol> toField = table.get_standard_type(StandardTypes::RangeEnumerable_ToField).as<FieldSymbol>();
			gmt::Ref<FieldSymbol> currentField = table.get_standard_type(StandardTypes::RangeEnumerable_CurrentField).as<FieldSymbol>();

			ObjectInstance instance = m_collector.allocate_instance(rangeShape);
			instance.get_field(fromField).write_integer(lower);
			instance.get_field(toField).write_integer(upper + (inclusive ? 1 : 0));
			instance.get_field(currentField).write_integer(lower - 1);

			push_instance(frame, instance);
			break;
		}

		case OpCode::LOADSTATICFIELD:
		{
			gmt::Ref<FieldSymbol> field = decoder.absorb_field_symbol();
			ObjectInstance storage = m_collector.get_static_storage(field);

			const TypeShape* shape = storage.get_shape();
			if (shape->is_reference())
			{
				std::byte* pointer = storage.as_pointer();
				frame->push_entry(shape, true, &pointer, sizeof(std::byte*));
				break;
			}

			frame->push_entry(shape, false, storage.get_memory(), shape->get_size());
			break;
		}

		case OpCode::STORESTATICFIELD:
		{
			gmt::Ref<FieldSymbol> field = decoder.absorb_field_symbol();
			ObjectInstance value = frame->pop_entry();
			ObjectInstance storage = m_collector.get_static_storage(field);

			const TypeShape* shape = storage.get_shape();
			if (shape->is_reference())
			{
				storage.write_pointer(value.get_memory());
				break;
			}

			std::memcpy(storage.get_memory(), value.get_memory(), shape->get_size());
			break;
		}

		case OpCode::LOADENUMFIELD:
		{
			gmt::Ref<FieldSymbol> field = decoder.absorb_field_symbol();
			const TypeShape* shape = resolve_shape(field->get_return_type());

			std::int64_t value = field->get_enum_value();
			frame->push_entry(shape, false, &value, sizeof(std::int64_t));
			break;
		}

		case OpCode::CALLMETHODSYMBOL:
		{
			gmt::Ref<MethodSymbol> method = decoder.absorb_method_symbol();

			std::size_t argumentCount;
			FrameLayout* layout = m_pragmatic.find_frame_layout(method);
			if (layout != nullptr)
				argumentCount = layout->get_argument_count();
			else
				argumentCount = native_argument_count(method);

			gmt::inline_vector<ObjectInstance, 8> arguments;
			for (std::size_t i = 0; i < argumentCount; ++i)
				arguments.push_back(frame->pop_entry());

			ObjectInstance result = invoke_method(method, arguments.get_vector());
			if (frame->get_interruption() == InterruptionReason::ExceptionRaised)
				break;

			if (!is_void_method(method))
				push_instance(frame, result);

			break;
		}

		case OpCode::CALL_METHOD_INTO:
		{
			gmt::Ref<MethodSymbol> method = decoder.absorb_method_symbol();
			std::uint16_t slot = decoder.absorb_uint16();

			std::size_t argumentCount;
			FrameLayout* layout = m_pragmatic.find_frame_layout(method);
			if (layout != nullptr)
				argumentCount = layout->get_argument_count();
			else
				argumentCount = native_argument_count(method);

			gmt::inline_vector<ObjectInstance, 8> arguments;
			for (std::size_t i = 0; i < argumentCount; ++i)
				arguments.push_back(frame->pop_entry());

			// a real caller slot, or the caller's own return slot when a tail
			// call forwards its result into the chained return storage
			std::byte* destinationEntry = slot == ReturnSlotSentinel
				? frame->get_return_entry()
				: frame->get_local_entry(slot);

			invoke_into(method, arguments.get_vector(), destinationEntry);
			break;
		}

		case OpCode::CALLINTERFACE:
		{
			gmt::Ref<MethodSymbol> interfaceMethod = decoder.absorb_method_symbol();

			std::size_t argumentCount;
			FrameLayout* layout = m_pragmatic.find_frame_layout(interfaceMethod);
			if (layout != nullptr)
				argumentCount = layout->get_argument_count();
			else
				argumentCount = native_argument_count(interfaceMethod);

			gmt::inline_vector<ObjectInstance, 8> arguments;
			for (std::size_t i = 0; i < argumentCount; ++i)
				arguments.push_back(frame->pop_entry());

			ObjectInstance receiver = arguments.get_vector().front();
			if (receiver.is_null())
			{
				raise_runtime_exception(L"NullReferenceException");
				break;
			}

			gmt::Ref<StructSymbol> receiverType = receiver.get_shape()->get_base_type();
			gmt::Ref<MethodSymbol> implementation = gmt::nullref;
			for (gmt::Ref<MethodSymbol> candidate : receiverType->methods)
			{
				if (candidate.is_null() || candidate->get_name() != interfaceMethod->get_name())
					continue;

				if (candidate->get_parameters().size() != interfaceMethod->get_parameters().size())
					continue;

				implementation = candidate;
				break;
			}

			if (implementation.is_null())
			{
				raise_runtime_exception(gmt::concat(L"interface method '", interfaceMethod->get_name(), L"' is not implemented"));
				break;
			}

			ObjectInstance result = invoke_method(implementation, arguments.get_vector());
			if (frame->get_interruption() == InterruptionReason::ExceptionRaised)
				break;

			if (!is_void_method(interfaceMethod))
				push_instance(frame, result);

			break;
		}

		case OpCode::NEWDELEGATE:
		{
			gmt::Ref<DelegateTypeSymbol> delegateType = decoder.absorb_delegate_type_symbol();
			gmt::Ref<MethodSymbol> method = decoder.absorb_method_symbol();

			const TypeShape* shape = resolve_shape(delegateType);
			ObjectInstance instance = m_collector.allocate_instance(shape, sizeof(void*) * 2);

			MethodSymbol* target = method.as_ptr();
			std::memcpy(instance.get_memory(), &target, sizeof(void*));

			std::byte* display = nullptr;
			std::memcpy(instance.get_memory() + sizeof(void*), &display, sizeof(void*));

			push_instance(frame, instance);
			break;
		}

		case OpCode::CALLDELEGATE:
		{
			gmt::Ref<DelegateTypeSymbol> delegateType = decoder.absorb_delegate_type_symbol();
			ObjectInstance delegate = frame->pop_entry();

			if (delegate.is_null())
			{
				raise_runtime_exception(L"NullReferenceException");
				break;
			}

			// a delegate value is a capture box instance: its single method is
			// the lambda body, invoked with the box itself as the receiver.
			// the eval entry may carry the slot's delegate type, so the true
			// box type comes from the object's own header when it is heap-backed
			const TypeShape* boxShape = delegate.get_shape();
			if (GarbageCollector::is_heap_pointer(delegate.get_memory()))
				boxShape = GarbageCollector::get_header(delegate.get_memory())->shape;

			gmt::Ref<StructSymbol> boxType = boxShape->get_base_type();
			if (boxType.is_null() || boxType->methods.empty())
			{
				raise_runtime_exception(L"delegate target is not callable");
				break;
			}

			gmt::Ref<MethodSymbol> target = boxType->methods.front();
			std::size_t parameterCount = delegateType->get_parameters().size();

			gmt::inline_vector<ObjectInstance, 8> arguments;
			arguments.push_back(delegate);
			for (std::size_t i = 0; i < parameterCount; ++i)
				arguments.push_back(frame->pop_entry());

			ObjectInstance result = invoke_method(target, arguments.get_vector());
			if (frame->get_interruption() == InterruptionReason::ExceptionRaised)
				break;

			gmt::Ref<StructSymbol> returnType = delegateType->get_return_type();
			if (!returnType.is_null() && returnType != m_model.get_table().get_void_primitive())
				push_instance(frame, result);

			break;
		}

		case OpCode::ISINSTANCE:
		{
			gmt::Ref<StructSymbol> type = decoder.absorb_type_symbol();
			ObjectInstance instance = frame->pop_entry();

			bool outcome = !instance.is_null() && is_instance_of(instance.get_shape(), type);
			frame->push_entry(m_primitiveShapes.boolean, false, &outcome, sizeof(bool));
			break;
		}

		case OpCode::CAST_AS:
		{
			gmt::Ref<StructSymbol> type = decoder.absorb_type_symbol();
			ObjectInstance instance = frame->pop_entry();

			if (!instance.is_null() && is_instance_of(instance.get_shape(), type))
			{
				push_instance(frame, instance);
				break;
			}

			std::byte* pointer = nullptr;
			frame->push_entry(nullptr, true, &pointer, sizeof(std::byte*));
			break;
		}

		case OpCode::CAST:
		{
			gmt::Ref<StructSymbol> type = decoder.absorb_type_symbol();
			ObjectInstance instance = frame->pop_entry();

			if (instance.is_null() || is_instance_of(instance.get_shape(), type))
			{
				push_instance(frame, instance);
				break;
			}

			raise_runtime_exception(L"InvalidCastException");
			break;
		}

		case OpCode::CASTPRIMITIVE:
		{
			gmt::Ref<StructSymbol> type = decoder.absorb_type_symbol();
			ObjectInstance value = frame->pop_entry();

			const TypeShape* targetShape = resolve_shape(type);
			if (value.get_shape() == targetShape)
			{
				push_instance(frame, value);
				break;
			}

			ScalarResult result;
			PrimitiveStatus status = PrimitiveMath::convert(m_primitiveShapes, value, type, targetShape, result);
			if (status != PrimitiveStatus::Success)
			{
				raise_runtime_exception(L"InvalidCastException");
				break;
			}

			push_scalar(frame, result);
			break;
		}

		case OpCode::ENTER_TRY:
		{
			std::size_t handlerOffset = decoder.absorb_jump();

			ExceptionHandlerRecord record;
			record.handlerOffset = handlerOffset;
			record.deferStackBase = frame->get_defer_stack().size();
			record.evalDepthBase = frame->get_eval_depth();
			frame->get_exception_handlers().push_back(record);
			break;
		}

		case OpCode::LEAVE_TRY:
		{
			frame->get_exception_handlers().pop_back();
			break;
		}

		case OpCode::END_CATCH:
		{
			frame->set_current_exception(ObjectInstance::null());
			break;
		}

		case OpCode::LOAD_CURRENT_EXCEPTION:
		{
			push_instance(frame, frame->get_current_exception());
			break;
		}

		case OpCode::STORE_CURRENT_EXCEPTION:
		{
			frame->set_current_exception(frame->pop_entry());
			break;
		}

		case OpCode::DEFER:
		{
			std::size_t target = decoder.absorb_jump();
			frame->get_defer_stack().push_back(target);
			break;
		}

		case OpCode::DEFER_DRAIN:
		{
			std::size_t count = decoder.absorb_jump();
			drain_defers(frame, decoder, count);
			break;
		}

		case OpCode::DEFER_BREAK:
		{
			throw std::runtime_error("DEFER_BREAK reached outside of a defer drain");
		}

		default:
		{
			throw std::runtime_error(gmt::wide_to_utf8(gmt::concat(L"unsupported opcode ", std::to_wstring(static_cast<std::uint16_t>(opcode)))));
		}
	}
}
