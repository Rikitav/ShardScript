#pragma once
#include <shard/Definitions.hpp>

#include <shard/runtime/CallStackFrame.hpp>
#include <shard/runtime/ObjectInstance.hpp>
#include <shard/runtime/VirtualMachine.hpp>

#include <shard/semantic/symbols/ConstructorSymbol.hpp>
#include <shard/semantic/symbols/MethodSymbol.hpp>
#include <shard/semantic/symbols/PropertySymbol.hpp>
#include <shard/semantic/symbols/StructSymbol.hpp>

#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <span>
#include <string_view>
#include <tuple>

namespace shard
{
	// Result of a capture-style invocation from native code: either a value
	// or the exception the callee raised, never both.
	class SHARD_API InvokeResult
	{
		ObjectInstance m_value;
		ObjectInstance m_exception;

	public:
		InvokeResult(ObjectInstance value, ObjectInstance exception);

		explicit operator bool() const { return (m_exception == nullptr); }
		bool is_ok() const { return (m_exception == nullptr); }

		ObjectInstance get_value() const;
		ObjectInstance get_exception() const;
	};

	// Callable RAII wrapper that pins a delegate (capture box) instance for
	// the wrapper's lifetime, so it can be invoked safely from native code.
	class SHARD_API DelegateRef
	{
		VirtualMachine* m_vm = nullptr;
		ObjectInstance m_delegate;
		bool m_pinned = false;

	public:
		DelegateRef() = default;
		DelegateRef(VirtualMachine& vm, ObjectInstance delegate);
		~DelegateRef();

		DelegateRef(const DelegateRef&) = delete;
		DelegateRef& operator=(const DelegateRef&) = delete;

		DelegateRef(DelegateRef&& other) noexcept;
		DelegateRef& operator=(DelegateRef&& other) noexcept;

		bool is_valid() const;
		ObjectInstance get_instance() const;

		ObjectInstance operator()(std::initializer_list<ObjectInstance> args = {}) const;
	};

	class SHARD_API MethodCallState
	{
		VirtualMachine& m_vm;
		CallStackFrame* m_frame;
		std::span<const ObjectInstance> m_arguments;
		ObjectInstance m_returnValue;
		alignas(8) std::byte m_returnBuffer[8] = {};

	public:
		MethodCallState(VirtualMachine& vm, CallStackFrame* frame, std::span<const ObjectInstance> arguments);

		VirtualMachine& get_vm() const;
		CallStackFrame* get_frame() const;

		std::size_t get_argument_count() const;
		ObjectInstance get_argument(std::size_t index) const;

		bool get_boolean_argument(std::size_t index) const;
		std::int64_t get_integer_argument(std::size_t index) const;
		double get_double_argument(std::size_t index) const;
		std::uintptr_t get_nint_argument(std::size_t index) const;
		wchar_t get_char_argument(std::size_t index) const;
		std::wstring_view get_string_argument(std::size_t index) const;

		template<typename... TArgs>
		std::tuple<TArgs...> get_arguments() const;

		void set_return(ObjectInstance value);
		void set_return_boolean(bool value);
		void set_return_integer(std::int64_t value);
		void set_return_nint(std::uintptr_t value);
		void set_return_double(double value);
		void set_return_char(wchar_t value);
		void set_return_byte(std::uint8_t value);
		void set_return_string(std::wstring_view value);

		ObjectInstance get_return_value() const;

		// Re-raises the exception on the callback's frame, so unwinding
		// continues in script code once the native returns.
		void propagate(ObjectInstance exception);

		InvokeResult try_invoke_method(MethodSymbol* method, std::span<const ObjectInstance> args);
		InvokeResult try_invoke_method(MethodSymbol* method, std::initializer_list<ObjectInstance> args);

		InvokeResult new_object(StructSymbol* type);
		InvokeResult new_object(StructSymbol* type, ConstructorSymbol* ctor, std::span<const ObjectInstance> args);
		InvokeResult new_object(StructSymbol* type, ConstructorSymbol* ctor, std::initializer_list<ObjectInstance> args);

		InvokeResult get_property(ObjectInstance obj, PropertySymbol* property);
		InvokeResult set_property(ObjectInstance obj, PropertySymbol* property, ObjectInstance value);

		DelegateRef wrap_delegate(ObjectInstance delegate);
	};
}

#include <shard/runtime/MethodCallState.impl.hpp>
