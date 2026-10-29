#include <shard/compilation/ByteCodeDecoder.hpp>
#include <shard/compilation/OperationCode.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/semantic/SyntaxSymbol.hpp>
#include <shard/semantic/symbols/ArrayTypeSymbol.hpp>
#include <shard/semantic/symbols/ConstructorSymbol.hpp>
#include <shard/semantic/symbols/DelegateTypeSymbol.hpp>
#include <shard/semantic/symbols/FieldSymbol.hpp>
#include <shard/semantic/symbols/MethodSymbol.hpp>
#include <shard/semantic/symbols/StructSymbol.hpp>

#include <cstring>
#include <stdexcept>

#include <gmt/Allocator.hpp>
#include <gmt/Arena.hpp>
#include <gmt/Containers.hpp>

using namespace shard;

namespace
{
	template <typename T>
	T absorb_value(const gmt::vector<std::byte>& code, std::size_t& ip, std::size_t end)
	{
		if (ip + sizeof(T) > end)
			throw std::out_of_range("ByteCodeDecoder: Tried to read out of code boundaries");

		T value{};
		std::memcpy(&value, &code[ip], sizeof(T));
		ip += sizeof(T);
		return value;
	}

	SyntaxSymbol* resolve_operand(std::size_t operand)
	{
		// operands are raw symbol pointers stored by the emitter; zero is null
		return reinterpret_cast<SyntaxSymbol*>(operand);
	}

	template <typename T>
	T* resolve_as(std::size_t operand, SyntaxKind kind)
	{
		SyntaxSymbol* symbol = resolve_operand(operand);
		if (symbol == nullptr)
			return nullptr;

		if (symbol->get_kind() != kind)
			return nullptr;

		return static_cast<T*>(symbol);
	}
}

ByteCodeDecoder::ByteCodeDecoder(
	const gmt::vector<std::byte>& code
) :
	m_code(code),
	m_base(0),
	m_end(m_code.size()),
	m_ip(0)
{ }

ByteCodeDecoder::ByteCodeDecoder(
	const gmt::vector<std::byte>& code,
	std::size_t start
) :
	m_code(code),
	m_base(start),
	m_end(m_code.size()),
	m_ip(start)
{
	if (m_base > m_end)
		throw std::out_of_range("ByteCodeDecoder: Method range out of code boundaries");
}

ByteCodeDecoder::ByteCodeDecoder(
	const gmt::vector<std::byte>& code,
	std::size_t start,
	std::size_t length
) :
	m_code(code),
	m_base(start),
	m_end(start + length),
	m_ip(start)
{
	if (m_end > m_code.size() || m_base > m_end)
		throw std::out_of_range("ByteCodeDecoder: Method range out of code boundaries");
}

bool ByteCodeDecoder::is_eof() const
{
	return m_ip >= m_end;
}

std::size_t ByteCodeDecoder::index() const
{
	return m_ip;
}

void ByteCodeDecoder::set_cursor(std::int64_t amount)
{
	if (amount < static_cast<std::int64_t>(m_base))
		throw std::out_of_range("ByteCodeDecoder: Tried to seek below code boundaries");

	if (amount > static_cast<std::int64_t>(m_end))
		throw std::out_of_range("ByteCodeDecoder: Tried to seek out of code boundaries");

	m_ip = static_cast<std::size_t>(amount);
}

void ByteCodeDecoder::skip_to_end()
{
	m_ip = m_end;
}

std::size_t ByteCodeDecoder::get_cursor() const
{
	return m_ip;
}

std::size_t ByteCodeDecoder::peek_jump_at(std::size_t offset) const
{
	if (offset + sizeof(std::size_t) > m_code.size())
		throw std::out_of_range("ByteCodeDecoder: Tried to read jump target out of code boundaries");

	std::size_t value{};
	std::memcpy(&value, &m_code[offset], sizeof(std::size_t));
	return value;
}

OpCode ByteCodeDecoder::absorb_opcode()
{
	return absorb_value<OpCode>(m_code, m_ip, m_end);
}

bool ByteCodeDecoder::absorb_boolean()
{
	return absorb_value<bool>(m_code, m_ip, m_end);
}

std::int64_t ByteCodeDecoder::absorb_int64()
{
	return absorb_value<std::int64_t>(m_code, m_ip, m_end);
}

std::intptr_t ByteCodeDecoder::absorb_int_ptr()
{
	return absorb_value<std::intptr_t>(m_code, m_ip, m_end);
}

double ByteCodeDecoder::absorb_double64()
{
	return absorb_value<double>(m_code, m_ip, m_end);
}

wchar_t ByteCodeDecoder::absorb_char16()
{
	return absorb_value<wchar_t>(m_code, m_ip, m_end);
}

std::size_t ByteCodeDecoder::absorb_string()
{
	return absorb_value<std::size_t>(m_code, m_ip, m_end);
}

std::uint8_t ByteCodeDecoder::absorb_uint8()
{
	return absorb_value<std::uint8_t>(m_code, m_ip, m_end);
}

std::uint16_t ByteCodeDecoder::absorb_variable_slot()
{
	return absorb_value<std::uint16_t>(m_code, m_ip, m_end);
}

std::uint16_t ByteCodeDecoder::absorb_uint16()
{
	return absorb_value<std::uint16_t>(m_code, m_ip, m_end);
}

std::uint32_t ByteCodeDecoder::absorb_uint32()
{
	return absorb_value<std::uint32_t>(m_code, m_ip, m_end);
}

std::uint32_t ByteCodeDecoder::absorb_field_slot()
{
	return absorb_value<std::uint32_t>(m_code, m_ip, m_end);
}

std::size_t ByteCodeDecoder::absorb_jump()
{
	return absorb_value<std::size_t>(m_code, m_ip, m_end);
}

SyntaxSymbol* ByteCodeDecoder::absorb_symbol()
{
	const std::size_t offset = absorb_value<std::size_t>(m_code, m_ip, m_end);
	return resolve_operand(offset);
}

StructSymbol* ByteCodeDecoder::absorb_type_symbol()
{
	const std::size_t offset = absorb_value<std::size_t>(m_code, m_ip, m_end);

	SyntaxSymbol* symbol = resolve_operand(offset);
	if (symbol == nullptr)
		return nullptr;

	if (!symbol->is_type())
		return nullptr;

	return static_cast<StructSymbol*>(symbol);
}

DelegateTypeSymbol* ByteCodeDecoder::absorb_delegate_type_symbol()
{
	const std::size_t offset = absorb_value<std::size_t>(m_code, m_ip, m_end);
	return resolve_as<DelegateTypeSymbol>(offset, SyntaxKind::DelegateType);
}

FieldSymbol* ByteCodeDecoder::absorb_field_symbol()
{
	const std::size_t offset = absorb_value<std::size_t>(m_code, m_ip, m_end);
	return resolve_as<FieldSymbol>(offset, SyntaxKind::FieldDeclaration);
}

ArrayTypeSymbol* ByteCodeDecoder::absorb_array_symbol()
{
	const std::size_t offset = absorb_value<std::size_t>(m_code, m_ip, m_end);
	return resolve_as<ArrayTypeSymbol>(offset, SyntaxKind::ArrayType);
}

MethodSymbol* ByteCodeDecoder::absorb_method_symbol()
{
	const std::size_t offset = absorb_value<std::size_t>(m_code, m_ip, m_end);

	SyntaxSymbol* symbol = resolve_operand(offset);
	if (symbol == nullptr)
		return nullptr;

	if (!symbol->is_method())
		return nullptr;

	return static_cast<MethodSymbol*>(symbol);
}

ConstructorSymbol* ByteCodeDecoder::absorb_constructor_symbol()
{
	const std::size_t offset = absorb_value<std::size_t>(m_code, m_ip, m_end);
	return resolve_as<ConstructorSymbol>(offset, SyntaxKind::ConstructorDeclaration);
}
