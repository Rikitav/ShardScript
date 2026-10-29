#include <shard/runtime/MethodCallState.hpp>

#include <shard/runtime/GarbageCollector.hpp>
#include <shard/runtime/VirtualMachine.hpp>

#include <shard/pragmatic/PragmaticModel.hpp>
#include <shard/pragmatic/TypeShape.hpp>

#include <shard/semantic/SemanticModel.hpp>
#include <shard/semantic/SymbolTable.hpp>
#include <shard/semantic/symbols/AccessorSymbol.hpp>
#include <shard/semantic/symbols/ConstructorSymbol.hpp>
#include <shard/semantic/symbols/GenericTypeSymbol.hpp>
#include <shard/semantic/symbols/MethodSymbol.hpp>
#include <shard/semantic/symbols/PropertySymbol.hpp>
#include <shard/semantic/symbols/StructSymbol.hpp>

#include <gmt/Allocator.hpp>
#include <gmt/InlineVector.hpp>

#include <stdexcept>
#include <utility>

using namespace shard;

namespace
{
	StructSymbol* constructed_definition(StructSymbol* type)
	{
		if (!(type == nullptr) && type->get_kind() == SyntaxKind::GenericType)
			return static_cast<GenericTypeSymbol*>(type)->get_underlaying_type();

		return type;
	}

	ConstructorSymbol* find_parameterless_constructor(StructSymbol* type)
	{
		StructSymbol* definition = constructed_definition(type);
		if ((definition == nullptr))
			return nullptr;

		for (ConstructorSymbol* ctor : definition->constructors)
		{
			if (ctor->get_parameters().empty())
				return ctor;
		}

		return nullptr;
	}
}

InvokeResult::InvokeResult(ObjectInstance value, ObjectInstance exception)
	: m_value(value)
	, m_exception(exception)
{
}

ObjectInstance InvokeResult::get_value() const
{
	return m_value;
}

ObjectInstance InvokeResult::get_exception() const
{
	return m_exception;
}

DelegateRef::DelegateRef(VirtualMachine& vm, ObjectInstance delegate)
	: m_vm(&vm)
	, m_delegate(delegate)
{
	if (!(m_delegate == nullptr))
	{
		m_vm->get_collector().pin(m_delegate);
		m_pinned = true;
	}
}

DelegateRef::~DelegateRef()
{
	if (m_pinned && m_vm != nullptr)
		m_vm->get_collector().unpin(1);
}

DelegateRef::DelegateRef(DelegateRef&& other) noexcept
	: m_vm(other.m_vm)
	, m_delegate(other.m_delegate)
	, m_pinned(other.m_pinned)
{
	other.m_vm = nullptr;
	other.m_delegate = ObjectInstance::null();
	other.m_pinned = false;
}

DelegateRef& DelegateRef::operator=(DelegateRef&& other) noexcept
{
	if (this == &other)
		return *this;

	if (m_pinned && m_vm != nullptr)
		m_vm->get_collector().unpin(1);

	m_vm = other.m_vm;
	m_delegate = other.m_delegate;
	m_pinned = other.m_pinned;

	other.m_vm = nullptr;
	other.m_delegate = ObjectInstance::null();
	other.m_pinned = false;
	return *this;
}

bool DelegateRef::is_valid() const
{
	return m_vm != nullptr && !(m_delegate == nullptr);
}

ObjectInstance DelegateRef::get_instance() const
{
	return m_delegate;
}

ObjectInstance DelegateRef::operator()(std::initializer_list<ObjectInstance> args) const
{
	if (!is_valid())
		throw std::runtime_error("DelegateRef is not valid");

	// a delegate value is a capture box instance: its single method is the
	// lambda body, invoked with the box itself as the receiver. the true box
	// type comes from the object's header when it is heap-backed
	const TypeShape* boxShape = m_delegate.get_shape();
	if (GarbageCollector::is_heap_pointer(m_delegate.get_memory()))
		boxShape = GarbageCollector::get_header(m_delegate.get_memory())->shape;

	StructSymbol* boxType = boxShape->get_base_type();
	if ((boxType == nullptr) || boxType->methods.empty())
		throw std::runtime_error("delegate target is not callable");

	MethodSymbol* target = boxType->methods.front();

	gmt::inline_vector<ObjectInstance, 8> fullArgs;
	fullArgs.push_back(m_delegate);
	for (const ObjectInstance& arg : args)
		fullArgs.push_back(arg);

	return m_vm->invoke_method(target, fullArgs.get_vector());
}

MethodCallState::MethodCallState(VirtualMachine& vm, CallStackFrame* frame, std::span<const ObjectInstance> arguments)
	: m_vm(vm)
	, m_frame(frame)
	, m_arguments(arguments)
	, m_returnValue(ObjectInstance::null())
{ }

VirtualMachine& MethodCallState::get_vm() const
{
	return m_vm;
}

CallStackFrame* MethodCallState::get_frame() const
{
	return m_frame;
}

std::size_t MethodCallState::get_argument_count() const
{
	return m_arguments.size();
}

ObjectInstance MethodCallState::get_argument(std::size_t index) const
{
	if (index >= m_arguments.size())
		throw std::out_of_range("argument index out of range");

	return m_arguments[index];
}

bool MethodCallState::get_boolean_argument(std::size_t index) const
{
	return get_argument(index).as_boolean();
}

std::int64_t MethodCallState::get_integer_argument(std::size_t index) const
{
	return get_argument(index).as_integer();
}

double MethodCallState::get_double_argument(std::size_t index) const
{
	return get_argument(index).as_double();
}

std::uintptr_t MethodCallState::get_nint_argument(std::size_t index) const
{
	return get_argument(index).as_nint();
}

wchar_t MethodCallState::get_char_argument(std::size_t index) const
{
	return get_argument(index).as_char();
}

std::wstring_view MethodCallState::get_string_argument(std::size_t index) const
{
	ObjectInstance instance = get_argument(index);
	if ((instance == nullptr))
		return std::wstring_view();

	std::int64_t length = instance.as_integer();
	const wchar_t* data = reinterpret_cast<const wchar_t*>(instance.get_memory() + sizeof(std::int64_t));
	return std::wstring_view(data, static_cast<std::size_t>(length));
}

void MethodCallState::set_return(ObjectInstance value)
{
	m_returnValue = value;
}

void MethodCallState::set_return_boolean(bool value)
{
	const TypeShape* shape = m_vm.get_primitive_shapes().boolean;
	std::memcpy(m_returnBuffer, &value, sizeof(bool));
	m_returnValue = ObjectInstance(shape, m_returnBuffer);
}

void MethodCallState::set_return_integer(std::int64_t value)
{
	const TypeShape* shape = m_vm.get_primitive_shapes().integer;
	std::memcpy(m_returnBuffer, &value, sizeof(std::int64_t));
	m_returnValue = ObjectInstance(shape, m_returnBuffer);
}

void MethodCallState::set_return_nint(std::uintptr_t value)
{
	const TypeShape* shape = m_vm.get_primitive_shapes().nint;
	std::memcpy(m_returnBuffer, &value, sizeof(std::uintptr_t));
	m_returnValue = ObjectInstance(shape, m_returnBuffer);
}

void MethodCallState::set_return_double(double value)
{
	const TypeShape* shape = m_vm.get_primitive_shapes().doubleReal;
	std::memcpy(m_returnBuffer, &value, sizeof(double));
	m_returnValue = ObjectInstance(shape, m_returnBuffer);
}

void MethodCallState::set_return_char(wchar_t value)
{
	const TypeShape* shape = m_vm.get_primitive_shapes().character;
	std::memcpy(m_returnBuffer, &value, sizeof(wchar_t));
	m_returnValue = ObjectInstance(shape, m_returnBuffer);
}

void MethodCallState::set_return_byte(std::uint8_t value)
{
	const TypeShape* shape = m_vm.get_primitive_shapes().byte;
	std::memcpy(m_returnBuffer, &value, sizeof(std::uint8_t));
	m_returnValue = ObjectInstance(shape, m_returnBuffer);
}

void MethodCallState::set_return_string(std::wstring_view value)
{
	m_returnValue = m_vm.create_string(value);
}

ObjectInstance MethodCallState::get_return_value() const
{
	return m_returnValue;
}

void MethodCallState::propagate(ObjectInstance exception)
{
	m_vm.raise_exception(exception);
}

InvokeResult MethodCallState::try_invoke_method(MethodSymbol* method, std::span<const ObjectInstance> args)
{
	ObjectInstance exception = ObjectInstance::null();
	ObjectInstance result = m_vm.try_invoke_method(method, args, exception);
	return InvokeResult(result, exception);
}

InvokeResult MethodCallState::try_invoke_method(MethodSymbol* method, std::initializer_list<ObjectInstance> args)
{
	return try_invoke_method(method, std::span<const ObjectInstance>(args.begin(), args.size()));
}

InvokeResult MethodCallState::new_object(StructSymbol* type)
{
	ConstructorSymbol* ctor = find_parameterless_constructor(type);
	if ((ctor == nullptr))
	{
		const TypeShape* shape = m_vm.get_pragmatic().find_or_create_type_shape(m_vm.get_model(), type);
		ObjectInstance instance = m_vm.get_collector().allocate_instance(shape);
		return InvokeResult(instance, ObjectInstance::null());
	}

	return new_object(type, ctor, {});
}

InvokeResult MethodCallState::new_object(StructSymbol* type, ConstructorSymbol* ctor, std::span<const ObjectInstance> args)
{
	const TypeShape* shape = m_vm.get_pragmatic().find_or_create_type_shape(m_vm.get_model(), type);
	ObjectInstance instance = m_vm.get_collector().allocate_instance(shape);

	gmt::inline_vector<ObjectInstance, 8> fullArgs;
	fullArgs.push_back(instance);
	for (const ObjectInstance& arg : args)
		fullArgs.push_back(arg);

	ObjectInstance exception = ObjectInstance::null();
	m_vm.try_invoke_method(ctor, fullArgs.get_vector(), exception);

	if (!(exception == nullptr))
		return InvokeResult(ObjectInstance::null(), exception);

	return InvokeResult(instance, ObjectInstance::null());
}

InvokeResult MethodCallState::new_object(StructSymbol* type, ConstructorSymbol* ctor, std::initializer_list<ObjectInstance> args)
{
	return new_object(type, ctor, std::span<const ObjectInstance>(args.begin(), args.size()));
}

InvokeResult MethodCallState::get_property(ObjectInstance obj, PropertySymbol* property)
{
	AccessorSymbol* getter = property->get_getter();
	if ((getter == nullptr))
		throw std::invalid_argument("property has no getter");

	const ObjectInstance args[1] = { obj };
	return try_invoke_method(getter, std::span<const ObjectInstance>(args, 1));
}

InvokeResult MethodCallState::set_property(ObjectInstance obj, PropertySymbol* property, ObjectInstance value)
{
	AccessorSymbol* setter = property->get_setter();
	if ((setter == nullptr))
		throw std::invalid_argument("property has no setter");

	const ObjectInstance args[2] = { obj, value };
	return try_invoke_method(setter, std::span<const ObjectInstance>(args, 2));
}

DelegateRef MethodCallState::wrap_delegate(ObjectInstance delegate)
{
	return DelegateRef(m_vm, delegate);
}