#include <shard/compilation/ByteCodeGenerator.h>
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

void ByteCodeGenerator::EmitNop(std::vector<std::byte>& code)
{
    AppendData(code, OpCode::Nop);
}

void ByteCodeGenerator::EmitHalt(std::vector<std::byte>& code)
{
    AppendData(code, OpCode::Halt);
}

void ByteCodeGenerator::EmitLoadConstNull(std::vector<std::byte>& code)
{
    AppendData(code, OpCode::LoadConst_Null);
}

void ByteCodeGenerator::EmitLoadConstBool(std::vector<std::byte>& code, bool value)
{
    AppendData(code, OpCode::LoadConst_Boolean);
    AppendData(code, value);
}

void ByteCodeGenerator::EmitLoadConstInt64(std::vector<std::byte>& code, int64_t value)
{
    AppendData(code, OpCode::LoadConst_Integer64);
    AppendData(code, value);
}

void ByteCodeGenerator::EmitLoadConstDouble64(std::vector<std::byte>& code, double value)
{
    AppendData(code, OpCode::LoadConst_Rational64);
    AppendData(code, value);
}

void ByteCodeGenerator::EmitLoadConstChar16(std::vector<std::byte>& code, wchar_t value)
{
    AppendData(code, OpCode::LoadConst_Char);
    AppendData(code, value);
}

void ByteCodeGenerator::EmitLoadConstString(std::vector<std::byte>& code, std::vector<std::byte>& data, const wchar_t* value)
{
    auto dataOrigin = data.size();
    AppendData(data, value);

    AppendData(code, OpCode::LoadConst_String);
    AppendData(code, dataOrigin);
}

void ByteCodeGenerator::EmitLoadVarible(std::vector<std::byte>& code, uint16_t index)
{
    AppendData(code, OpCode::LoadVariable);
    AppendData(code, index);
}

void ByteCodeGenerator::EmitStoreVarible(std::vector<std::byte>& code, uint16_t index)
{
    AppendData(code, OpCode::StoreVariable);
    AppendData(code, index);
}

void ByteCodeGenerator::EmitJump(std::vector<std::byte>& code, size_t jump)
{
    AppendData(code, OpCode::Jump);
    AppendData(code, jump);
}

void ByteCodeGenerator::EmitJumpTrue(std::vector<std::byte>& code, size_t jump)
{
    AppendData(code, OpCode::Jump_True);
    AppendData(code, jump);
}

void ByteCodeGenerator::EmitJumpFalse(std::vector<std::byte>& code, size_t jump)
{
    AppendData(code, OpCode::Jump_False);
    AppendData(code, jump);
}

void ByteCodeGenerator::EmitReturn(std::vector<std::byte>& code)
{
    AppendData(code, OpCode::Return);
}

void ByteCodeGenerator::EmitMathAdd(std::vector<std::byte>& code)
{
    AppendData(code, OpCode::Math_Addition);
}

void ByteCodeGenerator::EmitMathSub(std::vector<std::byte>& code)
{
    AppendData(code, OpCode::Math_Substraction);
}

void ByteCodeGenerator::EmitMathMult(std::vector<std::byte>& code)
{
    AppendData(code, OpCode::Math_Multiplication);
}

void ByteCodeGenerator::EmitMathDiv(std::vector<std::byte>& code)
{
    AppendData(code, OpCode::Math_Division);
}

void ByteCodeGenerator::EmitMathMod(std::vector<std::byte>& code)
{
    AppendData(code, OpCode::Math_Module);
}

void ByteCodeGenerator::EmitMathPow(std::vector<std::byte>& code)
{
    AppendData(code, OpCode::Math_Power);
}

void ByteCodeGenerator::EmitCompareEqual(std::vector<std::byte>& code)
{
    AppendData(code, OpCode::Compare_Equal);
}

void ByteCodeGenerator::EmitCompareNotEqual(std::vector<std::byte>& code)
{
    AppendData(code, OpCode::Compare_NotEqual);
}

void ByteCodeGenerator::EmitCompareGreater(std::vector<std::byte>& code)
{
    AppendData(code, OpCode::Compare_Greater);
}

void ByteCodeGenerator::EmitCompareGreaterOrEqual(std::vector<std::byte>& code)
{
    AppendData(code, OpCode::Compare_GreaterOrEqual);
}

void ByteCodeGenerator::EmitCompareLess(std::vector<std::byte>& code)
{
    AppendData(code, OpCode::Compare_Less);
}

void ByteCodeGenerator::EmitCompareLessOrEqual(std::vector<std::byte>& code)
{
    AppendData(code, OpCode::Compare_LessOrEqual);
}

void ByteCodeGenerator::EmitCompareNot(std::vector<std::byte>& code)
{
    AppendData(code, OpCode::Compare_Not);
}

void ByteCodeGenerator::EmitNewObject(std::vector<std::byte>& code, TypeSymbol* type)
{
    AppendData(code, OpCode::NewObject);
    AppendData(code, &type, sizeof(type));
}

void ByteCodeGenerator::EmitLoadField(std::vector<std::byte>& code, FieldSymbol* type)
{
    AppendData(code, OpCode::LoadField);
    AppendData(code, &type, sizeof(type));
}

void ByteCodeGenerator::EmitStoreField(std::vector<std::byte>& code, FieldSymbol* type)
{
    AppendData(code, OpCode::StoreField);
    AppendData(code, &type, sizeof(type));
}

void ByteCodeGenerator::EmitNewArray(std::vector<std::byte>& code, ArrayTypeSymbol* type)
{
    AppendData(code, OpCode::NewArray);
    AppendData(code, &type, sizeof(type));
}

void ByteCodeGenerator::EmitLoadArrayElement(std::vector<std::byte>& code)
{
    AppendData(code, OpCode::LoadArrayElement);
}

void ByteCodeGenerator::EmitStoreArrayElement(std::vector<std::byte>& code)
{
    AppendData(code, OpCode::StoreArrayElement);
}

void ByteCodeGenerator::EmitLoadStaticField(std::vector<std::byte>& code, FieldSymbol* type)
{
    AppendData(code, OpCode::LoadStaticField);
    AppendData(code, &type, sizeof(type));
}

void ByteCodeGenerator::EmitStoreStaticField(std::vector<std::byte>& code, FieldSymbol* type)
{
    AppendData(code, OpCode::StoreStaticField);
    AppendData(code, &type, sizeof(type));
}

void ByteCodeGenerator::EmitCallMethodSymbol(std::vector<std::byte>& code, MethodSymbol* method)
{
    AppendData(code, OpCode::CallMethodSymbol);
    AppendData(code, &method, sizeof(method));
}

void ByteCodeGenerator::EmitCallFunction(std::vector<std::byte>& code, MethodSymbolDelegate* func)
{
    AppendData(code, OpCode::CallFunction);
    AppendData(code, &func, sizeof(func));
}
