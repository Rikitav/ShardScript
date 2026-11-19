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

std::size_t ByteCodeDecoder::index() const
{
	return m_ip;
}

void ByteCodeDecoder::skip_to_end()
{
	m_ip = m_end;
}

std::size_t ByteCodeDecoder::peek_jump_at(std::size_t offset) const
{
	if (offset + sizeof(std::size_t) > m_code.size())
		throw std::out_of_range("ByteCodeDecoder: Tried to read jump target out of code boundaries");

	std::size_t value{};
	std::memcpy(&value, &m_code[offset], sizeof(std::size_t));
	return value;
}

SyntaxSymbol* ByteCodeDecoder::absorb_symbol()
{
	const std::size_t offset = absorb_value<std::size_t>();
	return resolve_operand(offset);
}

StructSymbol* ByteCodeDecoder::absorb_type_symbol()
{
	const std::size_t offset = absorb_value<std::size_t>();

	SyntaxSymbol* symbol = resolve_operand(offset);
	if (symbol == nullptr)
		return nullptr;

	if (!symbol->is_type())
		return nullptr;

	return static_cast<StructSymbol*>(symbol);
}

DelegateTypeSymbol* ByteCodeDecoder::absorb_delegate_type_symbol()
{
	const std::size_t offset = absorb_value<std::size_t>();
	return resolve_as<DelegateTypeSymbol>(offset, SyntaxKind::DelegateType);
}

FieldSymbol* ByteCodeDecoder::absorb_field_symbol()
{
	const std::size_t offset = absorb_value<std::size_t>();
	return resolve_as<FieldSymbol>(offset, SyntaxKind::FieldDeclaration);
}

ArrayTypeSymbol* ByteCodeDecoder::absorb_array_symbol()
{
	const std::size_t offset = absorb_value<std::size_t>();
	return resolve_as<ArrayTypeSymbol>(offset, SyntaxKind::ArrayType);
}

MethodSymbol* ByteCodeDecoder::absorb_method_symbol()
{
	const std::size_t offset = absorb_value<std::size_t>();

	SyntaxSymbol* symbol = resolve_operand(offset);
	if (symbol == nullptr)
		return nullptr;

	if (!symbol->is_method())
		return nullptr;

	return static_cast<MethodSymbol*>(symbol);
}

ConstructorSymbol* ByteCodeDecoder::absorb_constructor_symbol()
{
	const std::size_t offset = absorb_value<std::size_t>();
	return resolve_as<ConstructorSymbol>(offset, SyntaxKind::ConstructorDeclaration);
}
