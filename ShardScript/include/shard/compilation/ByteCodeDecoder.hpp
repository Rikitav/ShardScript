#pragma once
#include <shard/Definitions.hpp>
#include <shard/compilation/OperationCode.hpp>
#include <shard/semantic/SyntaxSymbol.hpp>
#include <gmt/Containers.hpp>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <stdexcept>
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

		// Operand reads happen several times per opcode, so they inline; the
		// symbol-resolving operands stay out of line because they need the
		// concrete symbol types.
		template <typename T>
		SHARD_FORCE_INLINE T absorb_value()
		{
			if (m_ip + sizeof(T) > m_end)
				throw std::out_of_range("ByteCodeDecoder: Tried to read out of code boundaries");

			T value{};
			std::memcpy(&value, &m_code[m_ip], sizeof(T));
			m_ip += sizeof(T);
			return value;
		}

	public:
		ByteCodeDecoder(const gmt::vector<std::byte>& code);
		ByteCodeDecoder(const gmt::vector<std::byte>& code, std::size_t start);
		ByteCodeDecoder(const gmt::vector<std::byte>& code, std::size_t start, std::size_t length);

		SHARD_FORCE_INLINE bool is_eof() const { return m_ip >= m_end; }
		std::size_t index() const;

		SHARD_FORCE_INLINE void set_cursor(std::int64_t amount)
		{
			if (amount < static_cast<std::int64_t>(m_base))
				throw std::out_of_range("ByteCodeDecoder: Tried to seek below code boundaries");

			if (amount > static_cast<std::int64_t>(m_end))
				throw std::out_of_range("ByteCodeDecoder: Tried to seek out of code boundaries");

			m_ip = static_cast<std::size_t>(amount);
		}

		void skip_to_end();
		SHARD_FORCE_INLINE std::size_t get_cursor() const { return m_ip; }
		std::size_t peek_jump_at(std::size_t offset) const;

		SHARD_FORCE_INLINE OpCode absorb_opcode() { return absorb_value<OpCode>(); }

		SHARD_FORCE_INLINE bool absorb_boolean() { return absorb_value<bool>(); }
		SHARD_FORCE_INLINE std::int64_t absorb_int64() { return absorb_value<std::int64_t>(); }
		SHARD_FORCE_INLINE std::intptr_t absorb_int_ptr() { return absorb_value<std::intptr_t>(); }
		SHARD_FORCE_INLINE double absorb_float64() { return absorb_value<double>(); }
		SHARD_FORCE_INLINE wchar_t absorb_char16() { return absorb_value<wchar_t>(); }
		SHARD_FORCE_INLINE std::size_t absorb_string() { return absorb_value<std::size_t>(); }

		SHARD_FORCE_INLINE std::uint8_t absorb_uint8() { return absorb_value<std::uint8_t>(); }
		SHARD_FORCE_INLINE std::uint16_t absorb_variable_slot() { return absorb_value<std::uint16_t>(); }
		SHARD_FORCE_INLINE std::uint16_t absorb_uint16() { return absorb_value<std::uint16_t>(); }
		SHARD_FORCE_INLINE std::uint32_t absorb_uint32() { return absorb_value<std::uint32_t>(); }
		SHARD_FORCE_INLINE std::uint32_t absorb_field_slot() { return absorb_value<std::uint32_t>(); }
		SHARD_FORCE_INLINE std::size_t absorb_jump() { return absorb_value<std::size_t>(); }

		SyntaxSymbol* absorb_symbol();
		StructSymbol* absorb_type_symbol();
		DelegateTypeSymbol* absorb_delegate_type_symbol();
		FieldSymbol* absorb_field_symbol();
		ArrayTypeSymbol* absorb_array_symbol();
		MethodSymbol* absorb_method_symbol();
		ConstructorSymbol* absorb_constructor_symbol();
	};
}