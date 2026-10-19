#include "StandardLibrary.hpp"

#include <shard/runtime/MethodCallState.hpp>

#include <shard/semantic/SemanticModel.hpp>
#include <shard/semantic/SymbolTable.hpp>
#include <shard/semantic/symbols/MethodSymbol.hpp>
#include <shard/semantic/symbols/NamespaceSymbol.hpp>
#include <shard/semantic/symbols/ParameterSymbol.hpp>
#include <shard/semantic/symbols/StructSymbol.hpp>

#include <iostream>

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

	gmt::Ref<MethodSymbol> register_native(SymbolTable& table, gmt::Ref<NamespaceSymbol> owner, const wchar_t* name, gmt::Ref<StructSymbol> parameterType, void (*callback)(MethodCallState&))
	{
		gmt::Ref<MethodSymbol> method = table.emplace<MethodSymbol>(name, owner);
		method->set_calling_convention(CallingConvention::ShardDecl);
		method->set_function_pointer(reinterpret_cast<void*>(callback));
		method->set_return_type(table.get_void_primitive());
		method->set_linking(LINK_STATIC);
		method->set_accesibility(ACS_PUBLIC);
		method->set_effects_computed(true);

		gmt::Ref<ParameterSymbol> parameter = table.emplace<ParameterSymbol>(L"value", method);
		parameter->set_type(parameterType);
		method->on_symbol_declared(parameter);

		owner->members.push_back(method);
		return method;
	}
}

namespace shard::interpreter
{
	void register_standard_library(SemanticModel& model)
	{
		SymbolTable& table = model.get_table();
		gmt::Ref<NamespaceSymbol> stdio = table.get_root_namespace()->lookup_or_create(L"stdio", table);

		register_native(table, stdio, L"print", table.get_integer_primitive(), &native_print_integer);
		register_native(table, stdio, L"println", table.get_integer_primitive(), &native_println_integer);
		register_native(table, stdio, L"print", table.get_string_primitive(), &native_print_string);
		register_native(table, stdio, L"println", table.get_string_primitive(), &native_println_string);
		register_native(table, stdio, L"print", table.get_double_primitive(), &native_print_double);
		register_native(table, stdio, L"println", table.get_double_primitive(), &native_println_double);
		register_native(table, stdio, L"print", table.get_boolean_primitive(), &native_print_boolean);
		register_native(table, stdio, L"println", table.get_boolean_primitive(), &native_println_boolean);
	}
}
