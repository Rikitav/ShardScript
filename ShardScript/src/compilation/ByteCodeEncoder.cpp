#include <shard/compilation/ByteCodeEncoder.h>
#include <shard/compilation/OperationCode.h>

#include <string>

using namespace shard;

static void AppendData(std::vector<std::byte>& code, const void* data, size_t size)
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

static void AppendData(std::vector<std::byte>& code, const wchar_t* string)
{
    AppendData(code, string, wcslen(string) * sizeof(wchar_t));
}

static void AppendData(std::vector<std::byte>& code, std::wstring& string)
{
    AppendData(code, string.data(), string.size() * sizeof(wchar_t));
}

template <typename T>
static void AppendData(std::vector<std::byte>& code, const T& value)
{
    static_assert(std::is_trivially_copyable_v<T>, "Type must be trivially copyable");
    AppendData(code, &value, sizeof(T));
}

void ByteCodeEncoder::EmitNop(std::vector<std::byte>& code)
{
    AppendData(code, OpCode::Nop);
}

void ByteCodeEncoder::EmitHalt(std::vector<std::byte>& code)
{
    AppendData(code, OpCode::Halt);
}

void ByteCodeEncoder::EmitLoadConstNull(std::vector<std::byte>& code)
{
    AppendData(code, OpCode::LoadConst_Null);
}

void ByteCodeEncoder::EmitLoadConstBool(std::vector<std::byte>& code, bool value)
{
    AppendData(code, OpCode::LoadConst_Boolean);
    AppendData(code, value);
}

void ByteCodeEncoder::EmitLoadConstInt64(std::vector<std::byte>& code, int64_t value)
{
    AppendData(code, OpCode::LoadConst_Integer64);
    AppendData(code, value);
}

void ByteCodeEncoder::EmitLoadConstDouble64(std::vector<std::byte>& code, double value)
{
    AppendData(code, OpCode::LoadConst_Rational64);
    AppendData(code, value);
}

void ByteCodeEncoder::EmitLoadConstChar16(std::vector<std::byte>& code, wchar_t value)
{
    AppendData(code, OpCode::LoadConst_Char);
    AppendData(code, value);
}

void ByteCodeEncoder::EmitLoadConstString(std::vector<std::byte>& code, std::vector<std::byte>& data, const wchar_t* value)
{
    size_t dataOrigin = data.size();
    AppendData(data, value);

    AppendData(code, OpCode::LoadConst_String);
    AppendData(code, dataOrigin);
}

void ByteCodeEncoder::EmitLoadVarible(std::vector<std::byte>& code, uint16_t index)
{
    AppendData(code, OpCode::LoadVariable);
    AppendData(code, index);
}

void ByteCodeEncoder::EmitStoreVarible(std::vector<std::byte>& code, uint16_t index)
{
    AppendData(code, OpCode::StoreVariable);
    AppendData(code, index);
}

void ByteCodeEncoder::EmitJump(std::vector<std::byte>& code, size_t jump)
{
    AppendData(code, OpCode::Jump);
    AppendData(code, jump);
}

void ByteCodeEncoder::EmitJumpTrue(std::vector<std::byte>& code, size_t jump)
{
    AppendData(code, OpCode::Jump_True);
    AppendData(code, jump);
}

void ByteCodeEncoder::EmitJumpFalse(std::vector<std::byte>& code, size_t jump)
{
    AppendData(code, OpCode::Jump_False);
    AppendData(code, jump);
}

void ByteCodeEncoder::EmitReturn(std::vector<std::byte>& code)
{
    AppendData(code, OpCode::Return);
}

void ByteCodeEncoder::EmitMathAdd(std::vector<std::byte>& code)
{
    AppendData(code, OpCode::Math_Addition);
}

void ByteCodeEncoder::EmitMathSub(std::vector<std::byte>& code)
{
    AppendData(code, OpCode::Math_Substraction);
}

void ByteCodeEncoder::EmitMathMult(std::vector<std::byte>& code)
{
    AppendData(code, OpCode::Math_Multiplication);
}

void ByteCodeEncoder::EmitMathDiv(std::vector<std::byte>& code)
{
    AppendData(code, OpCode::Math_Division);
}

void ByteCodeEncoder::EmitMathMod(std::vector<std::byte>& code)
{
    AppendData(code, OpCode::Math_Module);
}

void ByteCodeEncoder::EmitMathPow(std::vector<std::byte>& code)
{
    AppendData(code, OpCode::Math_Power);
}

void ByteCodeEncoder::EmitCompareEqual(std::vector<std::byte>& code)
{
    AppendData(code, OpCode::Compare_Equal);
}

void ByteCodeEncoder::EmitCompareNotEqual(std::vector<std::byte>& code)
{
    AppendData(code, OpCode::Compare_NotEqual);
}

void ByteCodeEncoder::EmitCompareGreater(std::vector<std::byte>& code)
{
    AppendData(code, OpCode::Compare_Greater);
}

void ByteCodeEncoder::EmitCompareGreaterOrEqual(std::vector<std::byte>& code)
{
    AppendData(code, OpCode::Compare_GreaterOrEqual);
}

void ByteCodeEncoder::EmitCompareLess(std::vector<std::byte>& code)
{
    AppendData(code, OpCode::Compare_Less);
}

void ByteCodeEncoder::EmitCompareLessOrEqual(std::vector<std::byte>& code)
{
    AppendData(code, OpCode::Compare_LessOrEqual);
}

void ByteCodeEncoder::EmitCompareNot(std::vector<std::byte>& code)
{
    AppendData(code, OpCode::Compare_Not);
}

void ByteCodeEncoder::EmitNewObject(std::vector<std::byte>& code, TypeSymbol* type)
{
    AppendData(code, OpCode::NewObject);
    AppendData(code, &type, sizeof(type));
}

void ByteCodeEncoder::EmitLoadField(std::vector<std::byte>& code, FieldSymbol* type)
{
    AppendData(code, OpCode::LoadField);
    AppendData(code, &type, sizeof(type));
}

void ByteCodeEncoder::EmitStoreField(std::vector<std::byte>& code, FieldSymbol* type)
{
    AppendData(code, OpCode::StoreField);
    AppendData(code, &type, sizeof(type));
}

void ByteCodeEncoder::EmitNewArray(std::vector<std::byte>& code, ArrayTypeSymbol* type)
{
    AppendData(code, OpCode::NewArray);
    AppendData(code, &type, sizeof(type));
}

void ByteCodeEncoder::EmitLoadArrayElement(std::vector<std::byte>& code)
{
    AppendData(code, OpCode::LoadArrayElement);
}

void ByteCodeEncoder::EmitStoreArrayElement(std::vector<std::byte>& code)
{
    AppendData(code, OpCode::StoreArrayElement);
}

void ByteCodeEncoder::EmitLoadStaticField(std::vector<std::byte>& code, FieldSymbol* type)
{
    AppendData(code, OpCode::LoadStaticField);
    AppendData(code, &type, sizeof(type));
}

void ByteCodeEncoder::EmitStoreStaticField(std::vector<std::byte>& code, FieldSymbol* type)
{
    AppendData(code, OpCode::StoreStaticField);
    AppendData(code, &type, sizeof(type));
}

void ByteCodeEncoder::EmitCallMethodSymbol(std::vector<std::byte>& code, MethodSymbol* method)
{
    AppendData(code, OpCode::CallMethodSymbol);
    AppendData(code, &method, sizeof(method));
}

/*
void ByteCodeEncoder::EmitCallFunction(std::vector<std::byte>& code, MethodSymbolDelegate* func)
{
    AppendData(code, OpCode::CallFunction);
    AppendData(code, &func, sizeof(func));
}
*/
