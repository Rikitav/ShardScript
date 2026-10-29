#pragma once
#include <shard/Definitions.hpp>
#include <shard/compilation/OperationCode.hpp>
#include <shard/semantic/SyntaxSymbol.hpp>
#include <gmt/Containers.hpp>

#include <cstddef>
#include <cstdint>
#include <vector>

namespace shard
{
	class StructSymbol;
	class FieldSymbol;
	class MethodSymbol;
	class ConstructorSymbol;
	class ArrayTypeSymbol;
	class DelegateTypeSymbol;

	class SHARD_API ByteCodeDecoder
	{
		const gmt::vector<std::byte>& m_code;
		std::size_t m_base;
		std::size_t m_end;
		std::size_t m_ip = 0;

	public:
		ByteCodeDecoder(const gmt::vector<std::byte>& code);
		ByteCodeDecoder(const gmt::vector<std::byte>& code, std::size_t start);
		ByteCodeDecoder(const gmt::vector<std::byte>& code, std::size_t start, std::size_t length);

		bool is_eof() const;
		std::size_t index() const;
		void set_cursor(std::int64_t amount);
		void skip_to_end();
		std::size_t get_cursor() const;
		std::size_t peek_jump_at(std::size_t offset) const;

		OpCode absorb_opcode();

		bool absorb_boolean();
		std::int64_t absorb_int64();
		std::intptr_t absorb_int_ptr();
		double absorb_double64();
		wchar_t absorb_char16();
		std::size_t absorb_string();

		std::uint8_t absorb_uint8();
		std::uint16_t absorb_variable_slot();
		std::uint16_t absorb_uint16();
		std::uint32_t absorb_uint32();
		std::uint32_t absorb_field_slot();
		std::size_t absorb_jump();

		SyntaxSymbol* absorb_symbol();
		StructSymbol* absorb_type_symbol();
		DelegateTypeSymbol* absorb_delegate_type_symbol();
		FieldSymbol* absorb_field_symbol();
		ArrayTypeSymbol* absorb_array_symbol();
		MethodSymbol* absorb_method_symbol();
		ConstructorSymbol* absorb_constructor_symbol();
	};
}
