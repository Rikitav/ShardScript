#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/compilation/OperationCode.h>

#include <shard/syntax/symbols/TypeSymbol.h>
#include <shard/syntax/symbols/FieldSymbol.h>
#include <shard/syntax/symbols/MethodSymbol.h>
#include <shard/syntax/symbols/ConstructorSymbol.h>
#include <shard/syntax/symbols/ArrayTypeSymbol.h>

#include <cstdint>
#include <cstddef>
#include <cstdio>
#include <vector>

namespace shard
{
	class SHARD_API ByteCodeDecoder
	{
		const std::vector<std::byte>& _code;
		size_t _ip = 0;

	public:
		inline ByteCodeDecoder(const std::vector<std::byte>& code) : _code(code) { }

		bool IsEOF();
		size_t Index() const;
		void SetCursor(fpos_t amount);
		void Return();

		OpCode AbsorbOpCode();

		bool AbsorbBoolean();
		int64_t AbsorbInt64();
		double AbsorbDouble64();
		wchar_t AbsorbChar16();
		size_t AbsorbString();

		uint16_t AbsorbVariableSlot();
		size_t AbsorbJump();
		
		TypeSymbol* AbsorbTypeSymbol();
		FieldSymbol* AbsorbFieldSymbol();
		ArrayTypeSymbol* AbsorbArraySymbol();
		MethodSymbol* AbsorbMethodSymbol();
		ConstructorSymbol* AbsorbConstructorSymbol();
		MethodSymbolDelegate AbsorbFunctionPtr();
	};
}