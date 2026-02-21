#include <shard/compilation/ByteCodeDecoder.hpp>
#include <shard/compilation/OperationCode.hpp>

#include <shard/syntax/symbols/ArrayTypeSymbol.hpp>
#include <shard/syntax/symbols/ConstructorSymbol.hpp>
#include <shard/syntax/symbols/FieldSymbol.hpp>
#include <shard/syntax/symbols/MethodSymbol.hpp>
#include <shard/syntax/symbols/TypeSymbol.hpp>

#include <stdexcept>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <vector>

using namespace shard;

template <typename T>
static bool ReadUnaligned(const std::vector<std::byte>& code, size_t& ip, T& value)
{
    if (ip + sizeof(T) > code.size())
        return false;

    memcpy(&value, &code[ip], sizeof(T));
    ip += sizeof(T);
    return true;
}

bool ByteCodeDecoder::IsEOF()
{
    return _ip >= _code.size();
}

size_t ByteCodeDecoder::Index() const
{
    return _ip;
}

void ByteCodeDecoder::SetCursor(fpos_t amount)
{
    // lower bound check
    if (amount < 0)
        throw new std::out_of_range("Tried to below code boundaries");

    // upper bound check
    if (amount > _code.size())
        throw new std::out_of_range("Tried to read out of code boundaries");

    // seeking
    _ip = amount;
}

void ByteCodeDecoder::Return()
{
    _ip = _code.size();
}

OpCode ByteCodeDecoder::AbsorbOpCode()
{
    OpCode value{};
    ReadUnaligned(_code, _ip, value);
    return value;
}

bool ByteCodeDecoder::AbsorbBoolean()
{
    bool value{};
    ReadUnaligned(_code, _ip, value);
    return value;
}

int64_t ByteCodeDecoder::AbsorbInt64()
{
    int64_t value{};
    ReadUnaligned(_code, _ip, value);
    return value;
}

double ByteCodeDecoder::AbsorbDouble64()
{
    double value{};
    ReadUnaligned(_code, _ip, value);
    return value;
}

wchar_t ByteCodeDecoder::AbsorbChar16()
{
    wchar_t value{};
    ReadUnaligned(_code, _ip, value);
    return value;
}

size_t ByteCodeDecoder::AbsorbString()
{
    size_t value{};
    ReadUnaligned(_code, _ip, value);
    return value;
}

uint16_t ByteCodeDecoder::AbsorbVariableSlot()
{
    uint16_t value{};
    ReadUnaligned(_code, _ip, value);
    return value;
}

size_t ByteCodeDecoder::AbsorbJump()
{
    size_t value{};
    ReadUnaligned(_code, _ip, value);
    return value;
}

TypeSymbol* ByteCodeDecoder::AbsorbTypeSymbol()
{
    TypeSymbol* value{};
    ReadUnaligned(_code, _ip, value);
    return value;
}

FieldSymbol* ByteCodeDecoder::AbsorbFieldSymbol()
{
    FieldSymbol* value{};
    ReadUnaligned(_code, _ip, value);
    return value;
}

ArrayTypeSymbol* ByteCodeDecoder::AbsorbArraySymbol()
{
    ArrayTypeSymbol* value{};
    ReadUnaligned(_code, _ip, value);
    return value;
}

MethodSymbol* ByteCodeDecoder::AbsorbMethodSymbol()
{
    MethodSymbol* value{};
    ReadUnaligned(_code, _ip, value);
    return value;
}

ConstructorSymbol* ByteCodeDecoder::AbsorbConstructorSymbol()
{
    ConstructorSymbol* value{};
    ReadUnaligned(_code, _ip, value);
    return value;
}

MethodSymbolDelegate ByteCodeDecoder::AbsorbFunctionPtr()
{
    MethodSymbolDelegate value{};
    ReadUnaligned(_code, _ip, value);
    return value;
}
