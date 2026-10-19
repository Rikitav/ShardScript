#pragma once
#include <shard/Definitions.hpp>

#include <shard/runtime/ObjectInstance.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

namespace shard
{
	class VirtualMachine;
	class CallStackFrame;

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
		wchar_t get_char_argument(std::size_t index) const;
		std::wstring_view get_string_argument(std::size_t index) const;

		void set_return(ObjectInstance value);
		void set_return_boolean(bool value);
		void set_return_integer(std::int64_t value);
		void set_return_double(double value);
		void set_return_char(wchar_t value);

		ObjectInstance get_return_value() const;
	};
}
