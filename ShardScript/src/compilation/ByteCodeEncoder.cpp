#include <shard/compilation/ByteCodeEncoder.h>
#include <shard/compilation/OperationCode.h>

#include <string>
#include <stdexcept>

using namespace shard;

void ByteCodeEncoder::PasteData(std::vector<std::byte>& code, size_t at, const void* data, size_t size)
{
    if (at + size > code.size())
        throw std::runtime_error("ByteCodeEncoder::PasteData: Out of bounds write!");

    std::memcpy(code.data() + at, data, size);
}

void ByteCodeEncoder::AppendData(std::vector<std::byte>& code, const void* data, size_t size)
{
    size_t current_size = code.size();
    size_t needed_capacity = current_size + size;

    if (needed_capacity > code.capacity())
    {
        size_t new_capacity = std::max(needed_capacity, code.capacity() * 2);
        code.reserve(new_capacity);
    }

    code.resize(needed_capacity);
    std::memcpy(code.data() + current_size, data, size);
}

static void AppendDataS(std::vector<std::byte>& code, const wchar_t* string)
{
    ByteCodeEncoder::AppendData(code, string, wcslen(string) * sizeof(wchar_t));
}

static void AppendDataS(std::vector<std::byte>& code, std::wstring& string)
{
    ByteCodeEncoder::AppendData(code, string.data(), string.size() * sizeof(wchar_t));
}

template <typename T>
static void AppendDataT(std::vector<std::byte>& code, const T& value)
{
    static_assert(std::is_trivially_copyable_v<T>, "Type must be trivially copyable");
    ByteCodeEncoder::AppendData(code, &value, sizeof(T));
}

template <typename T>
static void PasteData(std::vector<std::byte>& code, size_t at, const T& value)
{
    static_assert(std::is_trivially_copyable_v<T>, "Type must be trivially copyable");
    ByteCodeEncoder::PasteData(code, at, &value, sizeof(T));
}

void ByteCodeEncoder::EmitNop(std::vector<std::byte>& code)
{
    AppendDataT(code, OpCode::Nop);
}

void ByteCodeEncoder::EmitHalt(std::vector<std::byte>& code)
{
    AppendDataT(code, OpCode::Halt);
}

void ByteCodeEncoder::EmitLoadConstNull(std::vector<std::byte>& code)
{
    AppendDataT(code, OpCode::LoadConst_Null);
}

void ByteCodeEncoder::EmitLoadConstBool(std::vector<std::byte>& code, bool value)
{
    AppendDataT(code, OpCode::LoadConst_Boolean);
    AppendDataT(code, value);
}

void ByteCodeEncoder::EmitLoadConstInt64(std::vector<std::byte>& code, int64_t value)
{
    AppendDataT(code, OpCode::LoadConst_Integer64);
    AppendDataT(code, value);
}

void ByteCodeEncoder::EmitLoadConstDouble64(std::vector<std::byte>& code, double value)
{
    AppendDataT(code, OpCode::LoadConst_Rational64);
    AppendDataT(code, value);
}

void ByteCodeEncoder::EmitLoadConstChar16(std::vector<std::byte>& code, wchar_t value)
{
    AppendDataT(code, OpCode::LoadConst_Char);
    AppendDataT(code, value);
}

void ByteCodeEncoder::EmitLoadConstString(std::vector<std::byte>& code, std::vector<std::byte>& data, const wchar_t* value)
{
    size_t dataOrigin = data.size();
    AppendDataS(data, value);

    AppendDataT(code, OpCode::LoadConst_String);
    AppendDataT(code, dataOrigin);
}

void ByteCodeEncoder::EmitLoadVarible(std::vector<std::byte>& code, uint16_t index)
{
    AppendDataT(code, OpCode::LoadVariable);
    AppendDataT(code, index);
}

void ByteCodeEncoder::EmitStoreVarible(std::vector<std::byte>& code, uint16_t index)
{
    AppendDataT(code, OpCode::StoreVariable);
    AppendDataT(code, index);
}

void ByteCodeEncoder::EmitJump(std::vector<std::byte>& code, size_t jump)
{
    AppendDataT(code, OpCode::Jump);
    AppendDataT(code, jump);
}

void ByteCodeEncoder::EmitJumpTrue(std::vector<std::byte>& code, size_t jump)
{
    AppendDataT(code, OpCode::Jump_True);
    AppendDataT(code, jump);
}

void ByteCodeEncoder::EmitJumpFalse(std::vector<std::byte>& code, size_t jump)
{
    AppendDataT(code, OpCode::Jump_False);
    AppendDataT(code, jump);
}

void ByteCodeEncoder::EmitReturn(std::vector<std::byte>& code)
{
    AppendDataT(code, OpCode::Return);
}

void ByteCodeEncoder::EmitMathAdd(std::vector<std::byte>& code)
{
    AppendDataT(code, OpCode::Math_Addition);
}

void ByteCodeEncoder::EmitMathSub(std::vector<std::byte>& code)
{
    AppendDataT(code, OpCode::Math_Substraction);
}

void ByteCodeEncoder::EmitMathMult(std::vector<std::byte>& code)
{
    AppendDataT(code, OpCode::Math_Multiplication);
}

void ByteCodeEncoder::EmitMathDiv(std::vector<std::byte>& code)
{
    AppendDataT(code, OpCode::Math_Division);
}

void ByteCodeEncoder::EmitMathMod(std::vector<std::byte>& code)
{
    AppendDataT(code, OpCode::Math_Module);
}

void ByteCodeEncoder::EmitMathPow(std::vector<std::byte>& code)
{
    AppendDataT(code, OpCode::Math_Power);
}

void ByteCodeEncoder::EmitCompareEqual(std::vector<std::byte>& code)
{
    AppendDataT(code, OpCode::Compare_Equal);
}

void ByteCodeEncoder::EmitCompareNotEqual(std::vector<std::byte>& code)
{
    AppendDataT(code, OpCode::Compare_NotEqual);
}

void ByteCodeEncoder::EmitCompareGreater(std::vector<std::byte>& code)
{
    AppendDataT(code, OpCode::Compare_Greater);
}

void ByteCodeEncoder::EmitCompareGreaterOrEqual(std::vector<std::byte>& code)
{
    AppendDataT(code, OpCode::Compare_GreaterOrEqual);
}

void ByteCodeEncoder::EmitCompareLess(std::vector<std::byte>& code)
{
    AppendDataT(code, OpCode::Compare_Less);
}

void ByteCodeEncoder::EmitCompareLessOrEqual(std::vector<std::byte>& code)
{
    AppendDataT(code, OpCode::Compare_LessOrEqual);
}

void ByteCodeEncoder::EmitCompareNot(std::vector<std::byte>& code)
{
    AppendDataT(code, OpCode::Compare_Not);
}

void ByteCodeEncoder::EmitNewObject(std::vector<std::byte>& code, TypeSymbol* type)
{
    AppendDataT(code, OpCode::NewObject);
    AppendData(code, &type, sizeof(type));
}

void ByteCodeEncoder::EmitLoadField(std::vector<std::byte>& code, FieldSymbol* type)
{
    AppendDataT(code, OpCode::LoadField);
    AppendData(code, &type, sizeof(type));
}

void ByteCodeEncoder::EmitStoreField(std::vector<std::byte>& code, FieldSymbol* type)
{
    AppendDataT(code, OpCode::StoreField);
    AppendData(code, &type, sizeof(type));
}

void ByteCodeEncoder::EmitNewArray(std::vector<std::byte>& code, ArrayTypeSymbol* type)
{
    AppendDataT(code, OpCode::NewArray);
    AppendData(code, &type, sizeof(type));
}

void ByteCodeEncoder::EmitLoadArrayElement(std::vector<std::byte>& code)
{
    AppendDataT(code, OpCode::LoadArrayElement);
}

void ByteCodeEncoder::EmitStoreArrayElement(std::vector<std::byte>& code)
{
    AppendDataT(code, OpCode::StoreArrayElement);
}

void ByteCodeEncoder::EmitLoadStaticField(std::vector<std::byte>& code, FieldSymbol* type)
{
    AppendDataT(code, OpCode::LoadStaticField);
    AppendData(code, &type, sizeof(type));
}

void ByteCodeEncoder::EmitStoreStaticField(std::vector<std::byte>& code, FieldSymbol* type)
{
    AppendDataT(code, OpCode::StoreStaticField);
    AppendData(code, &type, sizeof(type));
}

void ByteCodeEncoder::EmitCallMethodSymbol(std::vector<std::byte>& code, MethodSymbol* method)
{
    AppendDataT(code, OpCode::CallMethodSymbol);
    AppendData(code, &method, sizeof(method));
}

/*
void ByteCodeEncoder::EmitCallFunction(std::vector<std::byte>& code, MethodSymbolDelegate* func)
{
    AppendDataT(code, OpCode::CallFunction);
    AppendData(code, &func, sizeof(func));
}
*/
