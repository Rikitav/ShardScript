#include <shard/compilation/ByteCodeEncoder.hpp>
#include <shard/compilation/OperationCode.hpp>
#include <shard/compilation/VirtualProgramImage.hpp>

#include <shard/semantic/SymbolTable.hpp>
#include <shard/semantic/symbols/ArrayTypeSymbol.hpp>
#include <shard/semantic/symbols/DelegateTypeSymbol.hpp>
#include <shard/semantic/symbols/FieldSymbol.hpp>
#include <shard/semantic/symbols/MethodSymbol.hpp>
#include <shard/semantic/symbols/StructSymbol.hpp>

#include <algorithm>
#include <cstring>
#include <cwchar>
#include <stdexcept>
#include <string>
#include <type_traits>

#include <gmt/Allocator.hpp>
#include <gmt/Containers.hpp>

using namespace shard;

namespace
{
	void append_data_string(gmt::vector<std::byte>& code, const wchar_t* string)
	{
		const std::size_t size = std::wcslen(string) * sizeof(wchar_t) + sizeof(wchar_t);
		ByteCodeEncoder::append_data(code, string, size);
	}

	template <typename T>
	void append_data_value(gmt::vector<std::byte>& code, const T& value)
	{
		static_assert(std::is_trivially_copyable_v<T>, "Type must be trivially copyable");
		ByteCodeEncoder::append_data(code, &value, sizeof(T));
	}

	template <typename T>
	void append_symbol_operand(gmt::vector<std::byte>& code, T* symbol)
	{
		// operands are raw symbol pointers; nullptr encodes as zero
		const std::size_t operand = reinterpret_cast<std::size_t>(symbol);
		append_data_value(code, operand);
	}
}

ByteCodeEncoder::ByteCodeEncoder(VirtualProgramImage& image)
	: m_image(image)
{ }

void ByteCodeEncoder::paste_data(gmt::vector<std::byte>& code, std::size_t at, const void* data, std::size_t size)
{
	if (at + size > code.size())
		throw std::runtime_error("ByteCodeEncoder::paste_data: Out of bounds write!");

	std::memcpy(code.data() + at, data, size);
}

void ByteCodeEncoder::append_data(gmt::vector<std::byte>& code, const void* data, std::size_t size)
{
	const std::size_t currentSize = code.size();
	const std::size_t neededCapacity = currentSize + size;

	if (neededCapacity > code.capacity())
	{
		const std::size_t newCapacity = (std::max)(neededCapacity, code.capacity() * 2);
		code.reserve(newCapacity);
	}

	code.resize(neededCapacity);
	std::memcpy(code.data() + currentSize, data, size);
}

void ByteCodeEncoder::emit_nop()
{
	append_data_value(m_image.get_text_section(), OpCode::NOP);
}

void ByteCodeEncoder::emit_halt()
{
	append_data_value(m_image.get_text_section(), OpCode::HALT);
}

void ByteCodeEncoder::emit_pop()
{
	append_data_value(m_image.get_text_section(), OpCode::POP);
}

void ByteCodeEncoder::emit_load_const_null()
{
	append_data_value(m_image.get_text_section(), OpCode::LOADCONST_NULL);
}

void ByteCodeEncoder::emit_load_const_bool(bool value)
{
	append_data_value(m_image.get_text_section(), OpCode::LOADCONST_BOOLEAN);
	append_data_value(m_image.get_text_section(), value);
}

void ByteCodeEncoder::emit_load_const_int8(std::int8_t value)
{
	append_data_value(m_image.get_text_section(), OpCode::LOADCONST_INTEGER8);
	append_data_value(m_image.get_text_section(), value);
}

void ByteCodeEncoder::emit_load_const_int64(std::int64_t value)
{
	append_data_value(m_image.get_text_section(), OpCode::LOADCONST_INTEGER64);
	append_data_value(m_image.get_text_section(), value);
}

void ByteCodeEncoder::emit_load_const_native_int(std::intptr_t value)
{
	append_data_value(m_image.get_text_section(), OpCode::LOADCONST_NATIVE_INTEGER);
	append_data_value(m_image.get_text_section(), value);
}

void ByteCodeEncoder::emit_load_const_double64(double value)
{
	append_data_value(m_image.get_text_section(), OpCode::LOADCONST_RATIONAL64);
	append_data_value(m_image.get_text_section(), value);
}

void ByteCodeEncoder::emit_load_const_char16(wchar_t value)
{
	append_data_value(m_image.get_text_section(), OpCode::LOADCONST_CHAR);
	append_data_value(m_image.get_text_section(), value);
}

void ByteCodeEncoder::emit_load_const_string(const wchar_t* value)
{
	const std::size_t length = std::wcslen(value);
	const std::size_t dataOrigin = m_image.get_data_section().size();
	append_data_string(m_image.get_data_section(), value);

	// the literal table keeps the length, so the runtime never scans the
	// data section with wcslen and can cache the interned instance per id
	const std::uint32_t literal = m_image.add_string_literal(dataOrigin, length);

	append_data_value(m_image.get_text_section(), OpCode::LOADCONST_STRING);
	append_data_value(m_image.get_text_section(), literal);
}

void ByteCodeEncoder::emit_default_value(StructSymbol* elementType, SymbolTable& table)
{
	if (elementType == table.get_integer_primitive())
	{
		emit_load_const_int8(0);
	}
	else if (elementType == table.get_byte_primitive())
	{
		emit_load_const_int64(0);
	}
	else if (elementType == table.get_nint_primitive())
	{
		emit_load_const_native_int(0);
	}
	else if (elementType == table.get_char_primitive())
	{
		emit_load_const_char16(L'\0');
	}
	else if (elementType == table.get_boolean_primitive())
	{
		emit_load_const_bool(false);
	}
	else if (elementType == table.get_double_primitive())
	{
		emit_load_const_double64(0.0);
	}
	else
	{
		emit_load_const_null();
	}
}

void ByteCodeEncoder::emit_duplicate()
{
	append_data_value(m_image.get_text_section(), OpCode::DUP);
}

void ByteCodeEncoder::emit_load_local(std::uint16_t index)
{
	append_data_value(m_image.get_text_section(), OpCode::LOAD_LOCAL);
	append_data_value(m_image.get_text_section(), index);
}

void ByteCodeEncoder::emit_store_local(std::uint16_t index)
{
	append_data_value(m_image.get_text_section(), OpCode::STORE_LOCAL);
	append_data_value(m_image.get_text_section(), index);
}

void ByteCodeEncoder::emit_load_arg(std::uint16_t index)
{
	append_data_value(m_image.get_text_section(), OpCode::LOAD_ARG);
	append_data_value(m_image.get_text_section(), index);
}

void ByteCodeEncoder::emit_store_arg(std::uint16_t index)
{
	append_data_value(m_image.get_text_section(), OpCode::STORE_ARG);
	append_data_value(m_image.get_text_section(), index);
}

void ByteCodeEncoder::emit_load_slot_addr(std::uint16_t slot)
{
	append_data_value(m_image.get_text_section(), OpCode::LOAD_SLOT_ADDR);
	append_data_value(m_image.get_text_section(), slot);
}

void ByteCodeEncoder::emit_jump(std::size_t jump)
{
	append_data_value(m_image.get_text_section(), OpCode::JUMP);
	append_data_value(m_image.get_text_section(), jump);
}

void ByteCodeEncoder::emit_jump_true(std::size_t jump)
{
	append_data_value(m_image.get_text_section(), OpCode::JUMP_TRUE);
	append_data_value(m_image.get_text_section(), jump);
}

void ByteCodeEncoder::emit_jump_false(std::size_t jump)
{
	append_data_value(m_image.get_text_section(), OpCode::JUMP_FALSE);
	append_data_value(m_image.get_text_section(), jump);
}

void ByteCodeEncoder::emit_branch_null(std::size_t jump)
{
	append_data_value(m_image.get_text_section(), OpCode::BR_NULL);
	append_data_value(m_image.get_text_section(), jump);
}

void ByteCodeEncoder::emit_jump_table(const gmt::vector<std::size_t>& targets)
{
	append_data_value(m_image.get_text_section(), OpCode::JUMP_TABLE);
	append_data_value(m_image.get_text_section(), static_cast<std::uint32_t>(targets.size()));
	for (const std::size_t target : targets)
		append_data_value(m_image.get_text_section(), target);
}

void ByteCodeEncoder::emit_return()
{
	append_data_value(m_image.get_text_section(), OpCode::RETURN);
}

void ByteCodeEncoder::emit_throw()
{
	append_data_value(m_image.get_text_section(), OpCode::THROW);
}

void ByteCodeEncoder::emit_enter_try(std::size_t handlerOffset)
{
	append_data_value(m_image.get_text_section(), OpCode::ENTER_TRY);
	append_data_value(m_image.get_text_section(), handlerOffset);
}

void ByteCodeEncoder::emit_leave_try()
{
	append_data_value(m_image.get_text_section(), OpCode::LEAVE_TRY);
}

void ByteCodeEncoder::emit_rethrow()
{
	append_data_value(m_image.get_text_section(), OpCode::RETHROW);
}

void ByteCodeEncoder::emit_end_catch()
{
	append_data_value(m_image.get_text_section(), OpCode::END_CATCH);
}

void ByteCodeEncoder::emit_load_current_exception()
{
	append_data_value(m_image.get_text_section(), OpCode::LOAD_CURRENT_EXCEPTION);
}

void ByteCodeEncoder::emit_store_current_exception()
{
	append_data_value(m_image.get_text_section(), OpCode::STORE_CURRENT_EXCEPTION);
}

void ByteCodeEncoder::emit_defer(std::size_t target)
{
	append_data_value(m_image.get_text_section(), OpCode::DEFER);
	append_data_value(m_image.get_text_section(), target);
}

void ByteCodeEncoder::emit_defer_break()
{
	append_data_value(m_image.get_text_section(), OpCode::DEFER_BREAK);
}

void ByteCodeEncoder::emit_defer_drain(std::size_t count)
{
	append_data_value(m_image.get_text_section(), OpCode::DEFER_DRAIN);
	append_data_value(m_image.get_text_section(), count);
}

void ByteCodeEncoder::emit_math_add()
{
	append_data_value(m_image.get_text_section(), OpCode::MATH_ADDITION);
}

void ByteCodeEncoder::emit_math_sub()
{
	append_data_value(m_image.get_text_section(), OpCode::MATH_SUBTRACT);
}

void ByteCodeEncoder::emit_math_mult()
{
	append_data_value(m_image.get_text_section(), OpCode::MATH_MULTIPLY);
}

void ByteCodeEncoder::emit_math_div()
{
	append_data_value(m_image.get_text_section(), OpCode::MATH_DIVISION);
}

void ByteCodeEncoder::emit_math_mod()
{
	append_data_value(m_image.get_text_section(), OpCode::MATH_MODULO);
}

void ByteCodeEncoder::emit_math_pow()
{
	append_data_value(m_image.get_text_section(), OpCode::MATH_POWER);
}

void ByteCodeEncoder::emit_math_negative()
{
	append_data_value(m_image.get_text_section(), OpCode::MATH_NEGATIVE);
}

void ByteCodeEncoder::emit_math_shl()
{
	append_data_value(m_image.get_text_section(), OpCode::MATH_SHL);
}

void ByteCodeEncoder::emit_math_shr()
{
	append_data_value(m_image.get_text_section(), OpCode::MATH_SHR);
}

void ByteCodeEncoder::emit_compare_equal()
{
	append_data_value(m_image.get_text_section(), OpCode::COMPARE_EQUAL);
}

void ByteCodeEncoder::emit_compare_not_equal()
{
	append_data_value(m_image.get_text_section(), OpCode::COMPARE_NOTEQUAL);
}

void ByteCodeEncoder::emit_compare_greater()
{
	append_data_value(m_image.get_text_section(), OpCode::COMPARE_GREATER);
}

void ByteCodeEncoder::emit_compare_greater_or_equal()
{
	append_data_value(m_image.get_text_section(), OpCode::COMPARE_GREATER_OR_EQUAL);
}

void ByteCodeEncoder::emit_compare_less()
{
	append_data_value(m_image.get_text_section(), OpCode::COMPARE_LESS);
}

void ByteCodeEncoder::emit_compare_less_or_equal()
{
	append_data_value(m_image.get_text_section(), OpCode::COMPARE_LESS_OR_EQUAL);
}

void ByteCodeEncoder::emit_logical_not()
{
	append_data_value(m_image.get_text_section(), OpCode::LOGICAL_NOT);
}

void ByteCodeEncoder::emit_logical_or()
{
	append_data_value(m_image.get_text_section(), OpCode::LOGICAL_OR);
}

void ByteCodeEncoder::emit_logical_and()
{
	append_data_value(m_image.get_text_section(), OpCode::LOGICAL_AND);
}

void ByteCodeEncoder::emit_alloc_heap(StructSymbol* type)
{
	append_data_value(m_image.get_text_section(), OpCode::ALLOC_HEAP);
	append_symbol_operand(m_image.get_text_section(), type);
}

void ByteCodeEncoder::emit_new_delegate(DelegateTypeSymbol* type, MethodSymbol* method)
{
	append_data_value(m_image.get_text_section(), OpCode::NEWDELEGATE);
	append_symbol_operand(m_image.get_text_section(), type);
	append_symbol_operand(m_image.get_text_section(), method);
}

void ByteCodeEncoder::emit_load_field(std::uint32_t slot)
{
	append_data_value(m_image.get_text_section(), OpCode::LOADFIELD);
	append_data_value(m_image.get_text_section(), slot);
}

void ByteCodeEncoder::emit_store_field(std::uint32_t slot)
{
	append_data_value(m_image.get_text_section(), OpCode::STOREFIELD);
	append_data_value(m_image.get_text_section(), slot);
}

void ByteCodeEncoder::emit_new_array(ArrayTypeSymbol* type)
{
	append_data_value(m_image.get_text_section(), OpCode::NEWARRAY);
	append_symbol_operand(m_image.get_text_section(), type);
}

void ByteCodeEncoder::emit_new_array_dynamic(ArrayTypeSymbol* type)
{
	append_data_value(m_image.get_text_section(), OpCode::NEWARRAY_DYNAMIC);
	append_symbol_operand(m_image.get_text_section(), type);
}

void ByteCodeEncoder::emit_create_range(StructSymbol* type)
{
	append_data_value(m_image.get_text_section(), OpCode::CREATERANGE);
	append_symbol_operand(m_image.get_text_section(), type);
}

void ByteCodeEncoder::emit_load_array_element()
{
	append_data_value(m_image.get_text_section(), OpCode::LOADARRAYELEMENT);
}

void ByteCodeEncoder::emit_store_array_element()
{
	append_data_value(m_image.get_text_section(), OpCode::STOREARRAYELEMENT);
}

void ByteCodeEncoder::emit_array_length()
{
	append_data_value(m_image.get_text_section(), OpCode::ARRAYLENGTH);
}

void ByteCodeEncoder::emit_load_static_field(FieldSymbol* field)
{
	append_data_value(m_image.get_text_section(), OpCode::LOADSTATICFIELD);
	append_symbol_operand(m_image.get_text_section(), field);
}

void ByteCodeEncoder::emit_store_static_field(FieldSymbol* field)
{
	append_data_value(m_image.get_text_section(), OpCode::STORESTATICFIELD);
	append_symbol_operand(m_image.get_text_section(), field);
}

void ByteCodeEncoder::emit_load_enum_field(FieldSymbol* field)
{
	append_data_value(m_image.get_text_section(), OpCode::LOADENUMFIELD);
	append_symbol_operand(m_image.get_text_section(), field);
}

void ByteCodeEncoder::emit_load_type_argument(std::uint16_t index, StructSymbol* type)
{
	append_data_value(m_image.get_text_section(), OpCode::LOAD_TYPEARGUMENT);
	append_data_value(m_image.get_text_section(), index);
	append_symbol_operand(m_image.get_text_section(), type);
}

void ByteCodeEncoder::emit_call_method_symbol(MethodSymbol* method)
{
	append_data_value(m_image.get_text_section(), OpCode::CALLMETHODSYMBOL);
	append_symbol_operand(m_image.get_text_section(), method);
}

void ByteCodeEncoder::emit_call_method_into(MethodSymbol* method, std::uint16_t slot)
{
	append_data_value(m_image.get_text_section(), OpCode::CALL_METHOD_INTO);
	append_symbol_operand(m_image.get_text_section(), method);
	append_data_value(m_image.get_text_section(), slot);
}

void ByteCodeEncoder::emit_return_placed()
{
	append_data_value(m_image.get_text_section(), OpCode::RETURN_PLACED);
}

void ByteCodeEncoder::emit_await()
{
	append_data_value(m_image.get_text_section(), OpCode::AWAIT);
}

void ByteCodeEncoder::emit_call_delegate(DelegateTypeSymbol* type)
{
	append_data_value(m_image.get_text_section(), OpCode::CALLDELEGATE);
	append_symbol_operand(m_image.get_text_section(), type);
}

void ByteCodeEncoder::emit_call_interface(MethodSymbol* interfaceMethod)
{
	append_data_value(m_image.get_text_section(), OpCode::CALLINTERFACE);
	append_symbol_operand(m_image.get_text_section(), interfaceMethod);
}

void ByteCodeEncoder::emit_is_instance(StructSymbol* type)
{
	append_data_value(m_image.get_text_section(), OpCode::ISINSTANCE);
	append_symbol_operand(m_image.get_text_section(), type);
}

void ByteCodeEncoder::emit_cast_as(StructSymbol* type)
{
	append_data_value(m_image.get_text_section(), OpCode::CAST_AS);
	append_symbol_operand(m_image.get_text_section(), type);
}

void ByteCodeEncoder::emit_cast(StructSymbol* type)
{
	append_data_value(m_image.get_text_section(), OpCode::CAST);
	append_symbol_operand(m_image.get_text_section(), type);
}

void ByteCodeEncoder::emit_cast_primitive(StructSymbol* type)
{
	append_data_value(m_image.get_text_section(), OpCode::CASTPRIMITIVE);
	append_symbol_operand(m_image.get_text_section(), type);
}
