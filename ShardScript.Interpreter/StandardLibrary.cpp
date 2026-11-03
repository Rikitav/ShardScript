#include "StandardLibrary.hpp"

#include <shard/runtime/MethodCallState.hpp>

#include <shard/pragmatic/PragmaticModel.hpp>
#include <shard/pragmatic/TypeShape.hpp>
#include <shard/runtime/GarbageCollector.hpp>
#include <shard/runtime/PrimitiveMath.hpp>
#include <shard/runtime/adapters/EnumerableAdapter.hpp>
#include <shard/semantic/SemanticModel.hpp>
#include <shard/semantic/StandardTypes.hpp>
#include <shard/semantic/SymbolTable.hpp>
#include <shard/semantic/symbols/MethodSymbol.hpp>
#include <shard/semantic/symbols/NamespaceSymbol.hpp>
#include <shard/semantic/symbols/ParameterSymbol.hpp>
#include <shard/semantic/symbols/StructSymbol.hpp>

#include <cstdint>
#include <cstdlib>
#include <string>
#include <cstring>
#include <iostream>
#include <optional>

using namespace shard;

namespace
{
	void native_print_integer(MethodCallState& state)
	{
		std::wcout << state.get_integer_argument(0);
	}

	void native_println_integer(MethodCallState& state)
	{
		std::wcout << state.get_integer_argument(0) << std::endl;
	}

	void native_print_string(MethodCallState& state)
	{
		std::wcout << state.get_string_argument(0);
	}

	void native_println_string(MethodCallState& state)
	{
		std::wcout << state.get_string_argument(0) << std::endl;
	}

	void native_print_double(MethodCallState& state)
	{
		std::wcout << state.get_double_argument(0);
	}

	void native_println_double(MethodCallState& state)
	{
		std::wcout << state.get_double_argument(0) << std::endl;
	}

	void native_print_boolean(MethodCallState& state)
	{
		std::wcout << (state.get_boolean_argument(0) ? L"true" : L"false");
	}

	void native_println_boolean(MethodCallState& state)
	{
		std::wcout << (state.get_boolean_argument(0) ? L"true" : L"false") << std::endl;
	}

	void write_newline()
	{
		std::wcout << std::endl;
	}

	void write_null(bool newline)
	{
		std::wcout << L"<NULL?>";

		if (newline)
			write_newline();
	}

	void write_printed_value(bool value)
	{
		std::wcout << (value ? L"true" : L"false");
	}

	void write_printed_value(std::int64_t value)
	{
		std::wcout << value;
	}

	void write_printed_value(std::uint8_t value)
	{
		std::wcout << static_cast<int>(value);
	}

	void write_printed_value(std::uintptr_t value)
	{
		std::wcout << value;
	}

	void write_printed_value(wchar_t value)
	{
		std::wcout << value;
	}

	void write_printed_value(double value)
	{
		std::wcout << value;
	}

	template<typename TValue>
	void print_nullable_primitive(MethodCallState& state, bool newline)
	{
		if (!state.get_nullable_has_value(0))
		{
			write_null(newline);
			return;
		}

		ObjectInstance slot = state.get_nullable_argument(0);
		TValue value{};
		std::memcpy(&value, slot.get_memory(), sizeof(TValue));
		write_printed_value(value);

		if (newline)
			write_newline();
	}

	template<typename TValue>
	void native_print_nullable(MethodCallState& state)
	{
		print_nullable_primitive<TValue>(state, false);
	}

	template<typename TValue>
	void native_println_nullable(MethodCallState& state)
	{
		print_nullable_primitive<TValue>(state, true);
	}

	void print_nullable_string(MethodCallState& state, bool newline)
	{
		if (!state.get_nullable_has_value(0))
		{
			write_null(newline);
			return;
		}

		std::wcout << state.get_nullable_argument(0).as_string();

		if (newline)
			write_newline();
	}

	void native_sum_sequence(MethodCallState& state)
	{
		std::int64_t total = 0;
		EnumerableAdapter sequence(state, state.get_argument(0));

		for (ObjectInstance value : sequence)
			total += value.as_integer();

		state.set_return_integer(total);
	}

	void native_parse_integer(MethodCallState& state)
	{
		std::wstring_view text = state.get_string_argument(0);

		if (!text.empty())
		{
			const std::wstring owned(text);
			wchar_t* end = nullptr;
			std::int64_t value = std::wcstoll(owned.c_str(), &end, 10);

			if ((end != nullptr) && (end == owned.c_str() + owned.size()))
			{
				state.set_return_nullable_integer(true, value);
				return;
			}
		}

		state.set_return_empty_nullable();
	}

	void native_print_nullable_string(MethodCallState& state)
	{
		print_nullable_string(state, false);
	}

	void native_println_nullable_string(MethodCallState& state)
	{
		print_nullable_string(state, true);
	}

	void print_printable(MethodCallState& state, ObjectInstance value, bool newline)
	{
		if (value == nullptr)
		{
			write_null(newline);
			return;
		}

		SymbolTable& table = state.get_vm().get_model().get_table();
		MethodSymbol* contract = static_cast<MethodSymbol*>(table.get_standard_type(StandardTypes::IPrintable_ToString));
		if (contract == nullptr)
		{
			state.get_vm().raise_runtime_exception(L"IPrintable.ToString is not available in this program image");
			return;
		}

		// the contract is a virtual member: the VM resolves the receiver's
		// implementation, so printing never looks a method up by name
		InvokeResult result = state.try_invoke_method(contract, { value });
		if (!result.is_ok())
		{
			state.propagate(result.get_exception());
			return;
		}

		std::wcout << result.get_value().as_string();

		if (newline)
			write_newline();
	}

	void native_print_printable(MethodCallState& state)
	{
		print_printable(state, state.get_argument(0), false);
	}

	void native_println_printable(MethodCallState& state)
	{
		print_printable(state, state.get_argument(0), true);
	}

	void print_nullable_printable(MethodCallState& state, bool newline)
	{
		if (!state.get_nullable_has_value(0))
		{
			write_null(newline);
			return;
		}

		print_printable(state, state.get_nullable_argument(0), newline);
	}

	void native_print_nullable_printable(MethodCallState& state)
	{
		print_nullable_printable(state, false);
	}

	void native_println_nullable_printable(MethodCallState& state)
	{
		print_nullable_printable(state, true);
	}

	MethodSymbol* register_native(SymbolTable& table, NamespaceSymbol* owner, const wchar_t* name, StructSymbol* parameterType, void (*callback)(MethodCallState&), StructSymbol* returnType = nullptr)
	{
		MethodSymbol* method = table.emplace<MethodSymbol>(name, owner);
		method->set_calling_convention(CallingConvention::ShardDecl);
		method->set_function_pointer(reinterpret_cast<void*>(callback));
		method->set_return_type(returnType != nullptr ? returnType : table.get_void_primitive());
		method->set_linking(LINK_STATIC);
		method->set_accesibility(ACS_PUBLIC);
		method->set_effects_computed(true);

		ParameterSymbol* parameter = table.emplace<ParameterSymbol>(L"value", method);
		parameter->set_type(parameterType);
		method->on_symbol_declared(parameter);

		owner->members.push_back(method);
		return method;
	}

	MethodSymbol* register_nullable_native(SymbolTable& table, NamespaceSymbol* owner, const wchar_t* name, StructSymbol* underlayingType, void (*callback)(MethodCallState&))
	{
		return register_native(table, owner, name, table.find_or_create_nullable_type(underlayingType), callback);
	}
}

namespace shard::interpreter
{
	void register_standard_library(SemanticModel& model)
	{
		SymbolTable& table = model.get_table();
		NamespaceSymbol* stdio = table.get_root_namespace()->lookup_or_create(L"stdio", table);

		StructSymbol* printable = static_cast<StructSymbol*>(table.get_standard_type(StandardTypes::IPrintable));

		register_native(table, stdio, L"print", table.get_integer_primitive(), &native_print_integer);
		register_native(table, stdio, L"println", table.get_integer_primitive(), &native_println_integer);
		register_native(table, stdio, L"print", table.get_string_primitive(), &native_print_string);
		register_native(table, stdio, L"println", table.get_string_primitive(), &native_println_string);
		register_native(table, stdio, L"print", table.get_double_primitive(), &native_print_double);
		register_native(table, stdio, L"println", table.get_double_primitive(), &native_println_double);
		register_native(table, stdio, L"print", table.get_boolean_primitive(), &native_print_boolean);
		register_native(table, stdio, L"println", table.get_boolean_primitive(), &native_println_boolean);

		register_native(table, stdio, L"print", printable, &native_print_printable);
		register_native(table, stdio, L"println", printable, &native_println_printable);
		register_nullable_native(table, stdio, L"print", printable, &native_print_nullable_printable);
		register_nullable_native(table, stdio, L"println", printable, &native_println_nullable_printable);

		register_nullable_native(table, stdio, L"print", table.get_boolean_primitive(), &native_print_nullable<bool>);
		register_nullable_native(table, stdio, L"println", table.get_boolean_primitive(), &native_println_nullable<bool>);
		register_nullable_native(table, stdio, L"print", table.get_integer_primitive(), &native_print_nullable<std::int64_t>);
		register_nullable_native(table, stdio, L"println", table.get_integer_primitive(), &native_println_nullable<std::int64_t>);
		register_nullable_native(table, stdio, L"print", table.get_double_primitive(), &native_print_nullable<double>);
		register_nullable_native(table, stdio, L"println", table.get_double_primitive(), &native_println_nullable<double>);
		register_nullable_native(table, stdio, L"print", table.get_char_primitive(), &native_print_nullable<wchar_t>);
		register_nullable_native(table, stdio, L"println", table.get_char_primitive(), &native_println_nullable<wchar_t>);
		register_nullable_native(table, stdio, L"print", table.get_byte_primitive(), &native_print_nullable<std::uint8_t>);
		register_nullable_native(table, stdio, L"println", table.get_byte_primitive(), &native_println_nullable<std::uint8_t>);
		register_nullable_native(table, stdio, L"print", table.get_nint_primitive(), &native_print_nullable<std::uintptr_t>);
		register_nullable_native(table, stdio, L"println", table.get_nint_primitive(), &native_println_nullable<std::uintptr_t>);
		register_native(table, stdio, L"parse_int", table.get_string_primitive(), &native_parse_integer, table.find_or_create_nullable_type(table.get_integer_primitive()));

		StructSymbol* enumerable = static_cast<StructSymbol*>(table.get_standard_type(StandardTypes::IEnumerable));
		StructSymbol* enumerableArguments[] = { table.get_integer_primitive() };
		StructSymbol* enumerableOfInteger = table.find_or_create_generic_type(enumerable, enumerableArguments);
		register_native(table, stdio, L"sum", enumerableOfInteger, &native_sum_sequence, table.get_integer_primitive());

		register_nullable_native(table, stdio, L"print", table.get_string_primitive(), &native_print_nullable_string);
		register_nullable_native(table, stdio, L"println", table.get_string_primitive(), &native_println_nullable_string);
	}
}
