#include <shard/semantic/SymbolTable.hpp>

#include <shard/semantic/symbols/MethodSymbol.hpp>

#include <shard/LibraryMetadata.hpp>
#include <shard/runtime/MethodCallState.hpp>

#include <gmt/Containers.hpp>

#include <string>

using namespace shard;

namespace
{
	static void primitive_boolean_to_string(MethodCallState& context)
	{
		context.set_return_string(context.get_boolean_argument(0) ? L"true" : L"false");
	}

	static void primitive_integer_to_string(MethodCallState& context)
	{
		context.set_return_string(std::to_wstring(context.get_integer_argument(0)));
	}

	static void primitive_double_to_string(MethodCallState& context)
	{
		context.set_return_string(std::to_wstring(context.get_double_argument(0)));
	}

	static void primitive_char_to_string(MethodCallState& context)
	{
		context.set_return_string(gmt::wstring(1, context.get_char_argument(0)));
	}

	static void primitive_byte_to_string(MethodCallState& context)
	{
		context.set_return_string(std::to_wstring(context.get_integer_argument(0)));
	}

	static void primitive_nint_to_string(MethodCallState& context)
	{
		context.set_return_string(std::to_wstring(context.get_integer_argument(0)));
	}

	static void primitive_string_to_string(MethodCallState& context)
	{
		context.set_return(context.get_argument(0));
	}

	static void make_printable(SymbolTable& table, gmt::Ref<StructSymbol> symbol, gmt::Ref<StructSymbol> printable, MethodSymbolDelegate callback)
	{
		gmt::Ref<MethodSymbol> toString = table.emplace<MethodSymbol>(L"ToString", symbol);
		toString->set_return_type(table.get_string_primitive());
		toString->set_accesibility(ACS_PUBLIC);
		toString->set_linking(LINK_INSTANCE);
		toString->set_calling_convention(CallingConvention::ShardDecl);
		toString->set_function_pointer(reinterpret_cast<void*>(callback));
		toString->set_effects_computed(true);

		symbol->methods.push_back(toString);
		symbol->interfaces.push_back(printable);
	}
}

void SymbolTable::resolve_primitive_printables()
{
	gmt::Ref<StructSymbol> printable = get_standard_type(StandardTypes::IPrintable).as<StructSymbol>();

	make_printable(*this, m_boolean, printable, &primitive_boolean_to_string);
	make_printable(*this, m_integer, printable, &primitive_integer_to_string);
	make_printable(*this, m_double, printable, &primitive_double_to_string);
	make_printable(*this, m_char, printable, &primitive_char_to_string);
	make_printable(*this, m_byte, printable, &primitive_byte_to_string);
	make_printable(*this, m_nativeInteger, printable, &primitive_nint_to_string);
	make_printable(*this, m_string, printable, &primitive_string_to_string);
}
