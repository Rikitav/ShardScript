#pragma once
#include <shard/Definitions.hpp>

#include <shard/runtime/NativeHandlers.hpp>
#include <shard/runtime/ObjectInstance.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

namespace shard
{
	class VirtualMachine;
	class CallStackFrame;
	class MethodSymbol;

	class SHARD_API AsyncToken
	{
		VirtualMachine* m_vm = nullptr;
		ObjectInstance m_taskState;

		void reset();

	public:
		AsyncToken() = default;
		AsyncToken(VirtualMachine& vm, ObjectInstance taskState);
		~AsyncToken();

		AsyncToken(const AsyncToken&) = delete;
		AsyncToken& operator=(const AsyncToken&) = delete;
		AsyncToken(AsyncToken&& other) noexcept;
		AsyncToken& operator=(AsyncToken&& other) noexcept;

		bool is_valid() const;
		ObjectInstance get_task_state() const;

		void complete();
		void complete(std::int64_t value);
		void complete(bool value);
		void complete(std::wstring_view value);
		void complete(ObjectInstance value);
		void fail(std::wstring_view errorMessage);
		void fail(ObjectInstance exception);
	};

	class SHARD_API AsyncFuncCallState
	{
		VirtualMachine& m_vm;
		CallStackFrame* m_frame;
		MethodSymbol* m_method;
		std::span<const ObjectInstance> m_arguments;
		ObjectInstance m_taskState;
		bool m_finished = false;

	public:
		AsyncFuncCallState(VirtualMachine& vm, CallStackFrame* frame, std::span<const ObjectInstance> arguments, MethodSymbol* method, ObjectInstance taskState);

		VirtualMachine& get_vm() const;
		CallStackFrame* get_frame() const;
		MethodSymbol* get_method() const;
		ObjectInstance get_task_state() const;
		bool is_finished() const;

		std::size_t get_argument_count() const;
		ObjectInstance get_argument(std::size_t index) const;
		bool get_boolean_argument(std::size_t index) const;
		std::int64_t get_integer_argument(std::size_t index) const;
		double get_double_argument(std::size_t index) const;
		std::uintptr_t get_nint_argument(std::size_t index) const;
		wchar_t get_char_argument(std::size_t index) const;
		std::wstring_view get_string_argument(std::size_t index) const;

		void complete();
		void complete(std::int64_t value);
		void complete(bool value);
		void complete(std::wstring_view value);
		void complete(ObjectInstance value);
		void complete_sync(FuncHandler handler);
		void fail(std::wstring_view errorMessage);
		void fail(ObjectInstance exception);

		[[nodiscard]] AsyncToken defer();
	};
}
