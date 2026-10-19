#include <shard/runtime/MethodCallState.hpp>

#include <shard/runtime/VirtualMachine.hpp>

#include <shard/pragmatic/PragmaticModel.hpp>
#include <shard/pragmatic/TypeShape.hpp>

#include <shard/semantic/SemanticModel.hpp>
#include <shard/semantic/SymbolTable.hpp>

#include <cstring>
#include <stdexcept>

using namespace shard;

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

wchar_t MethodCallState::get_char_argument(std::size_t index) const
{
	return get_argument(index).as_char();
}

std::wstring_view MethodCallState::get_string_argument(std::size_t index) const
{
	ObjectInstance instance = get_argument(index);
	if (instance.is_null())
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
	SymbolTable& table = m_vm.get_model().get_table();
	const TypeShape* shape = m_vm.get_pragmatic().find_or_create_type_shape(m_vm.get_model(), table.get_boolean_primitive());
	std::memcpy(m_returnBuffer, &value, sizeof(bool));
	m_returnValue = ObjectInstance(shape, m_returnBuffer);
}

void MethodCallState::set_return_integer(std::int64_t value)
{
	SymbolTable& table = m_vm.get_model().get_table();
	const TypeShape* shape = m_vm.get_pragmatic().find_or_create_type_shape(m_vm.get_model(), table.get_integer_primitive());
	std::memcpy(m_returnBuffer, &value, sizeof(std::int64_t));
	m_returnValue = ObjectInstance(shape, m_returnBuffer);
}

void MethodCallState::set_return_double(double value)
{
	SymbolTable& table = m_vm.get_model().get_table();
	const TypeShape* shape = m_vm.get_pragmatic().find_or_create_type_shape(m_vm.get_model(), table.get_double_primitive());
	std::memcpy(m_returnBuffer, &value, sizeof(double));
	m_returnValue = ObjectInstance(shape, m_returnBuffer);
}

void MethodCallState::set_return_char(wchar_t value)
{
	SymbolTable& table = m_vm.get_model().get_table();
	const TypeShape* shape = m_vm.get_pragmatic().find_or_create_type_shape(m_vm.get_model(), table.get_char_primitive());
	std::memcpy(m_returnBuffer, &value, sizeof(wchar_t));
	m_returnValue = ObjectInstance(shape, m_returnBuffer);
}

ObjectInstance MethodCallState::get_return_value() const
{
	return m_returnValue;
}
