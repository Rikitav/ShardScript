#include <shard/compilation/ByteCodeGenerator.h>
#include <shard/compilation/OperationCode.h>

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

void ByteCodeGenerator::EmitLoadConstBool(std::vector<std::byte>& code, bool value)
{
}

void ByteCodeGenerator::EmitLoadConstInt64(std::vector<std::byte>& code, int64_t value)
{
}

void ByteCodeGenerator::EmitLoadConstDouble64(std::vector<std::byte>& code, double value)
{
}

void ByteCodeGenerator::EmitLoadConstChar16(std::vector<std::byte>& code, wchar_t value)
{
}

void ByteCodeGenerator::EmitLoadConstString(std::vector<std::byte>& code, const wchar_t* value)
{
}

void ByteCodeGenerator::EmitLoadVarible(std::vector<std::byte>& code, size_t index)
{
}

void ByteCodeGenerator::EmitStoreVarible(std::vector<std::byte>& code, size_t index)
{
}

void ByteCodeGenerator::EmitJump(std::vector<std::byte>& code, size_t jump)
{
}

void ByteCodeGenerator::EmitJumpTrue(std::vector<std::byte>& code, size_t jump)
{
}

void ByteCodeGenerator::EmitJumpFalse(std::vector<std::byte>& code, size_t jump)
{
}

void ByteCodeGenerator::EmitReturn(std::vector<std::byte>& code)
{
}

void ByteCodeGenerator::EmitMathAdd(std::vector<std::byte>& code)
{
}

void ByteCodeGenerator::EmitMathSub(std::vector<std::byte>& code)
{
}

void ByteCodeGenerator::EmitMathMult(std::vector<std::byte>& code)
{
}

void ByteCodeGenerator::EmitMathDiv(std::vector<std::byte>& code)
{
}

void ByteCodeGenerator::EmitMathMod(std::vector<std::byte>& code)
{
}

void ByteCodeGenerator::EmitMathPow(std::vector<std::byte>& code)
{
}

void ByteCodeGenerator::EmitCompareEqual(std::vector<std::byte>& code)
{
}

void ByteCodeGenerator::EmitCompareNotEqual(std::vector<std::byte>& code)
{
}

void ByteCodeGenerator::EmitCompareGreater(std::vector<std::byte>& code)
{
}

void ByteCodeGenerator::EmitCompareGreaterOrEqual(std::vector<std::byte>& code)
{
}

void ByteCodeGenerator::EmitCompareLess(std::vector<std::byte>& code)
{
}

void ByteCodeGenerator::EmitCompareLessOrEqual(std::vector<std::byte>& code)
{
}

void ByteCodeGenerator::EmitCompareNot(std::vector<std::byte>& code)
{
}

void ByteCodeGenerator::EmitNewObject(std::vector<std::byte>& code, TypeSymbol* type)
{
}

void ByteCodeGenerator::EmitLoadField(std::vector<std::byte>& code, FieldSymbol* type)
{
}

void ByteCodeGenerator::EmitStoreField(std::vector<std::byte>& code, FieldSymbol* type)
{
}

void ByteCodeGenerator::EmitNewArray(std::vector<std::byte>& code, ArrayTypeSymbol* type)
{
}

void ByteCodeGenerator::EmitLoadArrayElement(std::vector<std::byte>& code)
{
}

void ByteCodeGenerator::EmitStoreArrayElement(std::vector<std::byte>& code)
{
}

void ByteCodeGenerator::EmitLoadStaticField(std::vector<std::byte>& code, FieldSymbol* type)
{
}

void ByteCodeGenerator::EmitStoreStaticField(std::vector<std::byte>& code, FieldSymbol* type)
{
}

void ByteCodeGenerator::EmitCallMethodSymbol(std::vector<std::byte>& code, MethodSymbol* method)
{
}

void ByteCodeGenerator::EmitCallFunction(std::vector<std::byte>& code, MethodSymbolDelegate* func)
{
}
