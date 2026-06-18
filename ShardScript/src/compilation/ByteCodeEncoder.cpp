#include <shard/compilation/ByteCodeEncoder.hpp>
#include <shard/compilation/OperationCode.hpp>

#include <shard/syntax/symbols/ArrayTypeSymbol.hpp>
#include <shard/syntax/symbols/FieldSymbol.hpp>
#include <shard/syntax/symbols/MethodSymbol.hpp>
#include <shard/syntax/symbols/TypeSymbol.hpp>
#include <shard/syntax/symbols/ConstructorSymbol.hpp>
#include <shard/syntax/symbols/DelegateTypeSymbol.hpp>

#include <string>
#include <stdexcept>
#include <cstring>
#include <algorithm>
#include <string.h>
#include <wchar.h>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <vector>

using namespace shard;

void ByteCodeEncoder::PasteData(std::vector<std::byte>& code, std::size_t at, const void* data, std::size_t size)
{
    if (at + size > code.size())
        throw std::runtime_error("ByteCodeEncoder::PasteData: Out of bounds write!");

    std::memcpy(code.data() + at, data, size);
}

void ByteCodeEncoder::AppendData(std::vector<std::byte>& code, const void* data, std::size_t size)
{
    std::size_t current_size = code.size();
    std::size_t needed_capacity = current_size + size;

    if (needed_capacity > code.capacity())
    {
        std::size_t new_capacity = (std::max)(needed_capacity, code.capacity() * 2);
        code.reserve(new_capacity);
    }

    code.resize(needed_capacity);
    std::memcpy(code.data() + current_size, data, size);
}

static void AppendDataS(std::vector<std::byte>& code, const wchar_t* string)
{
    ByteCodeEncoder::AppendData(code, string, wcslen(string) * sizeof(wchar_t) + sizeof(wchar_t));
}

static void AppendDataS(std::vector<std::byte>& code, std::wstring& string)
{
    ByteCodeEncoder::AppendData(code, string.data(), string.size() * sizeof(wchar_t) + sizeof(wchar_t));
}

template <typename T>
static void AppendDataT(std::vector<std::byte>& code, const T& value)
{
    static_assert(std::is_trivially_copyable_v<T>, "Type must be trivially copyable");
    ByteCodeEncoder::AppendData(code, &value, sizeof(T));
}

template <typename T>
static void PasteData(std::vector<std::byte>& code, std::size_t at, const T& value)
{
    static_assert(std::is_trivially_copyable_v<T>, "Type must be trivially copyable");
    ByteCodeEncoder::PasteData(code, at, &value, sizeof(T));
}

void ByteCodeEncoder::EmitNop(std::vector<std::byte>& code)
{
    AppendDataT(code, OpCode::NOP);
}

void ByteCodeEncoder::EmitHalt(std::vector<std::byte>& code)
{
    AppendDataT(code, OpCode::HALT);
}

void ByteCodeEncoder::EmitPop(std::vector<std::byte>& code)
{
    AppendDataT(code, OpCode::POPSTACK);
}

void ByteCodeEncoder::EmitLoadConstNull(std::vector<std::byte>& code)
{
    AppendDataT(code, OpCode::LOADCONST_NULL);
}

void ByteCodeEncoder::EmitLoadConstBool(std::vector<std::byte>& code, bool value)
{
    AppendDataT(code, OpCode::LOADCONST_BOOLEAN);
    AppendDataT(code, value);
}

void ByteCodeEncoder::EmitLoadConstInt64(std::vector<std::byte>& code, std::int64_t value)
{
    AppendDataT(code, OpCode::LOADCONST_INTEGER64);
    AppendDataT(code, value);
}

void ByteCodeEncoder::EmitLoadConstDouble64(std::vector<std::byte>& code, double value)
{
    AppendDataT(code, OpCode::LOADCONST_RATIONAL64);
    AppendDataT(code, value);
}

void ByteCodeEncoder::EmitLoadConstChar16(std::vector<std::byte>& code, wchar_t value)
{
    AppendDataT(code, OpCode::LOADCONST_CHAR);
    AppendDataT(code, value);
}

void ByteCodeEncoder::EmitLoadConstString(std::vector<std::byte>& code, std::vector<std::byte>& data, const wchar_t* value)
{
    std::size_t dataOrigin = data.size();
    AppendDataS(data, value);

    AppendDataT(code, OpCode::LOADCONST_STRING);
    AppendDataT(code, dataOrigin);
}

void ByteCodeEncoder::EmitDuplicate(std::vector<std::byte>& code)
{
    AppendDataT(code, OpCode::CREATEDUPLICATE);
}

void ByteCodeEncoder::EmitLoadVarible(std::vector<std::byte>& code, std::uint16_t index)
{
    AppendDataT(code, OpCode::LOADVARIABLE);
    AppendDataT(code, index);
}

void ByteCodeEncoder::EmitStoreVarible(std::vector<std::byte>& code, std::uint16_t index)
{
    AppendDataT(code, OpCode::STOREVARIABLE);
    AppendDataT(code, index);
}

void ByteCodeEncoder::EmitJump(std::vector<std::byte>& code, std::size_t jump)
{
    AppendDataT(code, OpCode::JUMP);
    AppendDataT(code, jump);
}

void ByteCodeEncoder::EmitJumpTrue(std::vector<std::byte>& code, std::size_t jump)
{
    AppendDataT(code, OpCode::JUMP_TRUE);
    AppendDataT(code, jump);
}

void ByteCodeEncoder::EmitJumpFalse(std::vector<std::byte>& code, std::size_t jump)
{
    AppendDataT(code, OpCode::JUMP_FALSE);
    AppendDataT(code, jump);
}

void ByteCodeEncoder::EmitReturn(std::vector<std::byte>& code)
{
    AppendDataT(code, OpCode::RETURN);
}

void shard::ByteCodeEncoder::EmitThrow(std::vector<std::byte>& code)
{
    AppendDataT(code, OpCode::THROW);
}

void ByteCodeEncoder::EmitMathAdd(std::vector<std::byte>& code)
{
    AppendDataT(code, OpCode::MATH_ADDITION);
}

void ByteCodeEncoder::EmitMathSub(std::vector<std::byte>& code)
{
    AppendDataT(code, OpCode::MATH_SUBSTRACTION);
}

void ByteCodeEncoder::EmitMathMult(std::vector<std::byte>& code)
{
    AppendDataT(code, OpCode::MATH_MULTIPLICATION);
}

void ByteCodeEncoder::EmitMathDiv(std::vector<std::byte>& code)
{
    AppendDataT(code, OpCode::MATH_DIVISION);
}

void ByteCodeEncoder::EmitMathMod(std::vector<std::byte>& code)
{
    AppendDataT(code, OpCode::MATH_MODULE);
}

void ByteCodeEncoder::EmitMathPow(std::vector<std::byte>& code)
{
    AppendDataT(code, OpCode::MATH_POWER);
}

void ByteCodeEncoder::EmitMathNegative(std::vector<std::byte>& code)
{
    AppendDataT(code, OpCode::MATH_NEGATIVE);
}

void ByteCodeEncoder::EmitMathPositive(std::vector<std::byte>& code)
{
    AppendDataT(code, OpCode::MATH_POSITIVE);
}

void ByteCodeEncoder::EmitCompareEqual(std::vector<std::byte>& code)
{
    AppendDataT(code, OpCode::COMPARE_EQUAL);
}

void ByteCodeEncoder::EmitCompareNotEqual(std::vector<std::byte>& code)
{
    AppendDataT(code, OpCode::COMPARE_NOTEQUAL);
}

void ByteCodeEncoder::EmitCompareGreater(std::vector<std::byte>& code)
{
    AppendDataT(code, OpCode::COMPARE_GREATER);
}

void ByteCodeEncoder::EmitCompareGreaterOrEqual(std::vector<std::byte>& code)
{
    AppendDataT(code, OpCode::COMPARE_GREATEROREQUAL);
}

void ByteCodeEncoder::EmitCompareLess(std::vector<std::byte>& code)
{
    AppendDataT(code, OpCode::COMPARE_LESS);
}

void ByteCodeEncoder::EmitCompareLessOrEqual(std::vector<std::byte>& code)
{
    AppendDataT(code, OpCode::COMPARE_LESSOREQUAL);
}

void ByteCodeEncoder::EmitLogicalNot(std::vector<std::byte>& code)
{
    AppendDataT(code, OpCode::LOGICAL_NOT);
}

void ByteCodeEncoder::EmitLogicalOr(std::vector<std::byte>& code)
{
    AppendDataT(code, OpCode::LOGICAL_OR);
}

void ByteCodeEncoder::EmitLogicalAnd(std::vector<std::byte>& code)
{
    AppendDataT(code, OpCode::LOGICAL_AND);
}

void ByteCodeEncoder::EmitLoadTypeArgument(std::vector<std::byte>& code, std::uint16_t index, TypeSymbol* type)
{
    AppendDataT(code, OpCode::LOAD_TYPEARGUMENT);
    AppendDataT(code, index);
    AppendData(code, &type, sizeof(type));
}

void ByteCodeEncoder::EmitNewObject(std::vector<std::byte>& code, TypeSymbol* type, ConstructorSymbol* ctor)
{
    AppendDataT(code, OpCode::NEWOBJECT);
    AppendData(code, &type, sizeof(type));
    AppendData(code, &ctor, sizeof(ctor));
}

void ByteCodeEncoder::EmitNewDelegate(std::vector<std::byte>& code, DelegateTypeSymbol* type)
{
    AppendDataT(code, OpCode::NEWDELEGATE);
    AppendData(code, &type, sizeof(type));
}

void ByteCodeEncoder::EmitLoadField(std::vector<std::byte>& code, FieldSymbol* type)
{
    AppendDataT(code, OpCode::LOADFIELD);
    AppendData(code, &type, sizeof(type));
}

void ByteCodeEncoder::EmitStoreField(std::vector<std::byte>& code, FieldSymbol* type)
{
    AppendDataT(code, OpCode::STOREFIELD);
    AppendData(code, &type, sizeof(type));
}

void ByteCodeEncoder::EmitNewArray(std::vector<std::byte>& code, ArrayTypeSymbol* type)
{
    AppendDataT(code, OpCode::NEWARRAY);
    AppendData(code, &type, sizeof(type));
}

void ByteCodeEncoder::EmitNewDynamicArray(std::vector<std::byte>& code, TypeSymbol* elementType)
{
    AppendDataT(code, OpCode::NEWDYNAMICARRAY);
    AppendData(code, &elementType, sizeof(elementType));
}

void ByteCodeEncoder::EmitCreateRange(std::vector<std::byte>& code, TypeSymbol* elementType)
{
    AppendDataT(code, OpCode::CREATERANGE);
    AppendData(code, &elementType, sizeof(elementType));
}

void ByteCodeEncoder::EmitLoadArrayElement(std::vector<std::byte>& code)
{
    AppendDataT(code, OpCode::LOADARRAYELEMENT);
}

void ByteCodeEncoder::EmitStoreArrayElement(std::vector<std::byte>& code)
{
    AppendDataT(code, OpCode::STOREARRAYELEMENT);
}

void ByteCodeEncoder::EmitArrayLength(std::vector<std::byte>& code)
{
    AppendDataT(code, OpCode::ARRAYLENGTH);
}

void ByteCodeEncoder::EmitLoadStaticField(std::vector<std::byte>& code, FieldSymbol* type)
{
    AppendDataT(code, OpCode::LOADSTATICFIELD);
    AppendData(code, &type, sizeof(type));
}

void ByteCodeEncoder::EmitStoreStaticField(std::vector<std::byte>& code, FieldSymbol* type)
{
    AppendDataT(code, OpCode::STORESTATICFIELD);
    AppendData(code, &type, sizeof(type));
}

void ByteCodeEncoder::EmitCallMethodSymbol(std::vector<std::byte>& code, MethodSymbol* method)
{
    AppendDataT(code, OpCode::CALLMETHODSYMBOL);
    AppendData(code, &method, sizeof(method));
}

void ByteCodeEncoder::EmitCallInterface(std::vector<std::byte>& code, MethodSymbol* interfaceMethod)
{
    AppendDataT(code, OpCode::CALLINTERFACE);
    AppendData(code, &interfaceMethod, sizeof(interfaceMethod));
}

void ByteCodeEncoder::EmitIsInstance(std::vector<std::byte>& code, TypeSymbol* type)
{
    AppendDataT(code, OpCode::ISINSTANCE);
    AppendData(code, &type, sizeof(type));
}

void ByteCodeEncoder::EmitCastInterface(std::vector<std::byte>& code, TypeSymbol* type)
{
    AppendDataT(code, OpCode::CASTINTERFACE);
    AppendData(code, &type, sizeof(type));
}

/*
void ByteCodeEncoder::EmitCallFunction(std::vector<std::byte>& code, MethodSymbolDelegate* func)
{
    AppendDataT(code, OpCode::CALLFUNCTION);
    AppendData(code, &func, sizeof(func));
}
*/
