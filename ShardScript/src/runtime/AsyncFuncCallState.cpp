#include <shard/runtime/AsyncFuncCallState.hpp>

#include <shard/runtime/GarbageCollector.hpp>
#include <shard/runtime/VirtualMachine.hpp>

#include <shard/pragmatic/PragmaticModel.hpp>
#include <shard/pragmatic/TypeShape.hpp>

#include <shard/semantic/SemanticModel.hpp>
#include <shard/semantic/SymbolTable.hpp>
#include <shard/semantic/symbols/StructSymbol.hpp>

#include <stdexcept>
#include <utility>

using namespace shard;

namespace
{
	ObjectInstance allocate_primitive(VirtualMachine& vm, StructSymbol* type)
	{
		const TypeShape* shape = vm.get_pragmatic().find_or_create_type_shape(vm.get_model(), type);
		return vm.get_collector().allocate_instance(shape);
	}

	ObjectInstance box_integer(VirtualMachine& vm, std::int64_t value)
	{
		ObjectInstance result = allocate_primitive(vm, vm.get_model().get_table().get_integer_primitive());
		result.write_integer(value);
		return result;
	}

	ObjectInstance box_boolean(VirtualMachine& vm, bool value)
	{
		ObjectInstance result = allocate_primitive(vm, vm.get_model().get_table().get_boolean_primitive());
		result.write_boolean(value);
		return result;
	}
}

AsyncToken::AsyncToken(VirtualMachine& vm, ObjectInstance taskState)
	: m_vm(&vm)
	, m_taskState(taskState)
{ }

AsyncToken::~AsyncToken()
{
	reset();
}

AsyncToken::AsyncToken(AsyncToken&& other) noexcept
	: m_vm(other.m_vm)
	, m_taskState(other.m_taskState)
{
	other.m_vm = nullptr;
	other.m_taskState = ObjectInstance::null();
}

AsyncToken& AsyncToken::operator=(AsyncToken&& other) noexcept
{
	if (this == &other)
		return *this;

	reset();
	m_vm = other.m_vm;
	m_taskState = other.m_taskState;
	other.m_vm = nullptr;
	other.m_taskState = ObjectInstance::null();
	return *this;
}

bool AsyncToken::is_valid() const
{
	return m_vm != nullptr && m_taskState != nullptr;
}

ObjectInstance AsyncToken::get_task_state() const
{
	return m_taskState;
}

void AsyncToken::reset()
{
	if (!is_valid())
		return;

	VirtualMachine* vm = m_vm;
	ObjectInstance state = m_taskState;
	m_vm = nullptr;
	m_taskState = ObjectInstance::null();
	vm->fail_task_state(state, vm->build_runtime_exception(L"Asynchronous operation was abandoned"));
}

void AsyncToken::complete()
{
	complete(ObjectInstance::null());
}

void AsyncToken::complete(std::int64_t value)
{
	if (!is_valid())
		return;

	VirtualMachine* vm = m_vm;
	ObjectInstance state = m_taskState;
	m_vm = nullptr;
	m_taskState = ObjectInstance::null();
	vm->complete_task_state(state, box_integer(*vm, value));
}

void AsyncToken::complete(bool value)
{
	if (!is_valid())
		return;

	VirtualMachine* vm = m_vm;
	ObjectInstance state = m_taskState;
	m_vm = nullptr;
	m_taskState = ObjectInstance::null();
	vm->complete_task_state(state, box_boolean(*vm, value));
}

void AsyncToken::complete(std::wstring_view value)
{
	if (!is_valid())
		return;

	VirtualMachine* vm = m_vm;
	ObjectInstance state = m_taskState;
	m_vm = nullptr;
	m_taskState = ObjectInstance::null();
	vm->complete_task_state(state, vm->create_string(value));
}

void AsyncToken::complete(ObjectInstance value)
{
	if (!is_valid())
		return;

	VirtualMachine* vm = m_vm;
	ObjectInstance state = m_taskState;
	m_vm = nullptr;
	m_taskState = ObjectInstance::null();
	vm->complete_task_state(state, value);
}

void AsyncToken::fail(std::wstring_view errorMessage)
{
	if (!is_valid())
		return;

	VirtualMachine* vm = m_vm;
	ObjectInstance state = m_taskState;
	m_vm = nullptr;
	m_taskState = ObjectInstance::null();
	vm->fail_task_state(state, vm->build_runtime_exception(errorMessage));
}

void AsyncToken::fail(ObjectInstance exception)
{
	if (!is_valid())
		return;

	VirtualMachine* vm = m_vm;
	ObjectInstance state = m_taskState;
	m_vm = nullptr;
	m_taskState = ObjectInstance::null();
	vm->fail_task_state(state, exception);
}

AsyncFuncCallState::AsyncFuncCallState(VirtualMachine& vm, CallStackFrame* frame, std::span<const ObjectInstance> arguments, MethodSymbol* method, ObjectInstance taskState)
	: m_vm(vm)
	, m_frame(frame)
	, m_method(method)
	, m_arguments(arguments)
	, m_taskState(taskState)
{ }

VirtualMachine& AsyncFuncCallState::get_vm() const { return m_vm; }
CallStackFrame* AsyncFuncCallState::get_frame() const { return m_frame; }
MethodSymbol* AsyncFuncCallState::get_method() const { return m_method; }
ObjectInstance AsyncFuncCallState::get_task_state() const { return m_taskState; }
bool AsyncFuncCallState::is_finished() const { return m_finished; }
std::size_t AsyncFuncCallState::get_argument_count() const { return m_arguments.size(); }

ObjectInstance AsyncFuncCallState::get_argument(std::size_t index) const
{
	if (index >= m_arguments.size())
		throw std::out_of_range("argument index out of range");

	return m_arguments[index];
}

bool AsyncFuncCallState::get_boolean_argument(std::size_t index) const { return get_argument(index).as_boolean(); }
std::int64_t AsyncFuncCallState::get_integer_argument(std::size_t index) const { return get_argument(index).as_integer(); }
double AsyncFuncCallState::get_double_argument(std::size_t index) const { return get_argument(index).as_double(); }
std::uintptr_t AsyncFuncCallState::get_nint_argument(std::size_t index) const { return get_argument(index).as_nint(); }
wchar_t AsyncFuncCallState::get_char_argument(std::size_t index) const { return get_argument(index).as_char(); }

std::wstring_view AsyncFuncCallState::get_string_argument(std::size_t index) const
{
	ObjectInstance instance = get_argument(index);
	if (instance == nullptr)
		return std::wstring_view();

	std::int64_t length = instance.as_integer();
	const wchar_t* data = reinterpret_cast<const wchar_t*>(instance.get_memory() + sizeof(std::int64_t));
	return std::wstring_view(data, static_cast<std::size_t>(length));
}

void AsyncFuncCallState::complete()
{
	complete(ObjectInstance::null());
}

void AsyncFuncCallState::complete(std::int64_t value)
{
	if (m_finished)
		return;

	m_finished = true;
	m_vm.complete_task_state(m_taskState, box_integer(m_vm, value));
}

void AsyncFuncCallState::complete(bool value)
{
	if (m_finished)
		return;

	m_finished = true;
	m_vm.complete_task_state(m_taskState, box_boolean(m_vm, value));
}

void AsyncFuncCallState::complete(std::wstring_view value)
{
	if (m_finished)
		return;

	m_finished = true;
	m_vm.complete_task_state(m_taskState, m_vm.create_string(value));
}

void AsyncFuncCallState::complete(ObjectInstance value)
{
	if (m_finished)
		return;

	m_finished = true;
	m_vm.complete_task_state(m_taskState, value);
}

void AsyncFuncCallState::complete_sync(FuncHandler handler)
{
	if (m_finished)
		return;

	if (handler == nullptr)
	{
		fail(L"Native async completion callback cannot be null");
		return;
	}

	ObjectInstance exception = ObjectInstance::null();
	ObjectInstance* previousCapture = m_vm.m_exceptionCapture;
	m_vm.m_exceptionCapture = &exception;
	ObjectInstance result = m_vm.invoke_native_callback(handler, m_method, m_arguments);
	m_vm.m_exceptionCapture = previousCapture;

	if (exception != nullptr)
		fail(exception);
	else
		complete(result);
}

void AsyncFuncCallState::fail(std::wstring_view errorMessage)
{
	if (m_finished)
		return;

	m_finished = true;
	m_vm.fail_task_state(m_taskState, m_vm.build_runtime_exception(errorMessage));
}

void AsyncFuncCallState::fail(ObjectInstance exception)
{
	if (m_finished)
		return;

	m_finished = true;
	m_vm.fail_task_state(m_taskState, exception);
}

AsyncToken AsyncFuncCallState::defer()
{
	if (m_finished)
		throw std::logic_error("native async operation was already completed or deferred");

	m_finished = true;
	return AsyncToken(m_vm, m_taskState);
}
