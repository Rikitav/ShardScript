#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/syntax/symbols/TypeSymbol.h>
#include <shard/syntax/symbols/FieldSymbol.h>
#include <shard/syntax/symbols/MethodSymbol.h>
#include <shard/syntax/symbols/ArrayTypeSymbol.h>

namespace shard
{
	class SHARD_API ByteCodeEncoder
	{
	public:
		inline ByteCodeEncoder() { }

		static void AppendData(std::vector<std::byte>& code, const void* data, size_t size);
		static void PasteData(std::vector<std::byte>& code, size_t at, const void* data, size_t size);

		void EmitNop(std::vector<std::byte>& code);
		void EmitHalt(std::vector<std::byte>& code);

		void EmitLoadConstNull(std::vector<std::byte>& code);
		void EmitLoadConstBool(std::vector<std::byte>& code, bool value);
		void EmitLoadConstInt64(std::vector<std::byte>& code, int64_t value);
		void EmitLoadConstDouble64(std::vector<std::byte>& code, double value);
		void EmitLoadConstChar16(std::vector<std::byte>& code, wchar_t value);
		void EmitLoadConstString(std::vector<std::byte>& code, std::vector<std::byte>& data, const wchar_t* value);

		void EmitLoadVarible(std::vector<std::byte>& code, uint16_t index);
		void EmitStoreVarible(std::vector<std::byte>& code, uint16_t index);

		void EmitJump(std::vector<std::byte>& code, size_t jump);
		void EmitJumpTrue(std::vector<std::byte>& code, size_t jump);
		void EmitJumpFalse(std::vector<std::byte>& code, size_t jump);
		void EmitReturn(std::vector<std::byte>& code);

		void EmitMathAdd(std::vector<std::byte>& code);
		void EmitMathSub(std::vector<std::byte>& code);
		void EmitMathMult(std::vector<std::byte>& code);
		void EmitMathDiv(std::vector<std::byte>& code);
		void EmitMathMod(std::vector<std::byte>& code);
		void EmitMathPow(std::vector<std::byte>& code);

		void EmitCompareEqual(std::vector<std::byte>& code);
		void EmitCompareNotEqual(std::vector<std::byte>& code);
		void EmitCompareGreater(std::vector<std::byte>& code);
		void EmitCompareGreaterOrEqual(std::vector<std::byte>& code);
		void EmitCompareLess(std::vector<std::byte>& code);
		void EmitCompareLessOrEqual(std::vector<std::byte>& code);
		void EmitCompareNot(std::vector<std::byte>& code);

		void EmitNewObject(std::vector<std::byte>& code, TypeSymbol* type);
		void EmitLoadField(std::vector<std::byte>& code, FieldSymbol* type);
		void EmitStoreField(std::vector<std::byte>& code, FieldSymbol* type);

		void EmitNewArray(std::vector<std::byte>& code, ArrayTypeSymbol* type);
		void EmitLoadArrayElement(std::vector<std::byte>& code);
		void EmitStoreArrayElement(std::vector<std::byte>& code);

		void EmitLoadStaticField(std::vector<std::byte>& code, FieldSymbol* type);
		void EmitStoreStaticField(std::vector<std::byte>& code, FieldSymbol* type);

		void EmitCallMethodSymbol(std::vector<std::byte>& code, MethodSymbol* method);
		//void EmitCallFunction(std::vector<std::byte>& code, MethodSymbolDelegate* func);
	};
}
