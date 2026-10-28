#pragma once
#include <shard/Definitions.hpp>

#include <shard/semantic/SymbolTable.hpp>
#include <shard/semantic/symbols/StructSymbol.hpp>
#include <shard/semantic/symbols/FieldSymbol.hpp>
#include <shard/semantic/symbols/MethodSymbol.hpp>
#include <shard/semantic/symbols/ArrayTypeSymbol.hpp>
#include <shard/semantic/symbols/DelegateTypeSymbol.hpp>

#include <shard/compilation/VirtualProgramImage.hpp>

#include <gmt/Containers.hpp>
#include <gmt/Ref.hpp>

#include <cstddef>
#include <cstdint>
#include <vector>

namespace shard
{
	class SHARD_API ByteCodeEncoder
	{
		VirtualProgramImage& m_image;

	public:
		explicit ByteCodeEncoder(VirtualProgramImage& image);

		static void append_data(gmt::vector<std::byte>& code, const void* data, std::size_t size);
		static void paste_data(gmt::vector<std::byte>& code, std::size_t at, const void* data, std::size_t size);

		void emit_nop();
		void emit_halt();

		void emit_pop();

		void emit_load_const_null();
		void emit_load_const_bool(bool value);
		void emit_load_const_int8(std::int8_t value);
		void emit_load_const_int64(std::int64_t value);
		void emit_load_const_native_int(std::intptr_t value);
		void emit_load_const_double64(double value);
		void emit_load_const_char16(wchar_t value);
		void emit_load_const_string(const wchar_t* value);
		void emit_default_value(StructSymbol* elementType, SymbolTable& table);
		void emit_duplicate();

		void emit_load_local(std::uint16_t index);
		void emit_store_local(std::uint16_t index);
		void emit_load_arg(std::uint16_t index);
		void emit_store_arg(std::uint16_t index);
		void emit_load_slot_addr(std::uint16_t slot);

		void emit_jump(std::size_t jump);
		void emit_jump_true(std::size_t jump);
		void emit_jump_false(std::size_t jump);
		void emit_branch_null(std::size_t jump);
		void emit_jump_table(const gmt::vector<std::size_t>& targets);

		void emit_return();
		void emit_throw();
		void emit_enter_try(std::size_t handlerOffset);
		void emit_leave_try();
		void emit_rethrow();
		void emit_end_catch();
		void emit_load_current_exception();
		void emit_store_current_exception();

		void emit_defer(std::size_t target);
		void emit_defer_break();
		void emit_defer_drain(std::size_t count);

		void emit_math_add();
		void emit_math_sub();
		void emit_math_mult();
		void emit_math_div();
		void emit_math_mod();
		void emit_math_pow();
		void emit_math_negative();
		void emit_math_shl();
		void emit_math_shr();

		void emit_compare_equal();
		void emit_compare_not_equal();
		void emit_compare_greater();
		void emit_compare_greater_or_equal();
		void emit_compare_less();
		void emit_compare_less_or_equal();

		void emit_logical_not();
		void emit_logical_or();
		void emit_logical_and();

		void emit_alloc_heap(StructSymbol* type);
		void emit_new_delegate(DelegateTypeSymbol* type, MethodSymbol* method);
		void emit_load_field(std::uint32_t slot);
		void emit_store_field(std::uint32_t slot);

		void emit_new_array(ArrayTypeSymbol* type);
		void emit_new_array_dynamic(ArrayTypeSymbol* type);
		void emit_create_range(StructSymbol* type);
		void emit_load_array_element();
		void emit_store_array_element();
		void emit_array_length();

		void emit_load_static_field(FieldSymbol* field);
		void emit_store_static_field(FieldSymbol* field);
		void emit_load_enum_field(FieldSymbol* field);

		void emit_load_type_argument(std::uint16_t index, StructSymbol* type);
		void emit_call_method_symbol(MethodSymbol* method);
		void emit_call_method_into(MethodSymbol* method, std::uint16_t slot);
		void emit_call_delegate(DelegateTypeSymbol* type);
		void emit_call_interface(MethodSymbol* interfaceMethod);
		void emit_return_placed();
		void emit_is_instance(StructSymbol* type);
		void emit_cast_as(StructSymbol* type);
		void emit_cast(StructSymbol* type);
		void emit_cast_primitive(StructSymbol* type);
	};
}
