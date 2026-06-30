#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/compilation/OperationCode.hpp>

#include <shard/semantic/symbols/TypeSymbol.hpp>
#include <shard/semantic/symbols/FieldSymbol.hpp>
#include <shard/semantic/symbols/MethodSymbol.hpp>
#include <shard/semantic/symbols/ConstructorSymbol.hpp>
#include <shard/semantic/symbols/ArrayTypeSymbol.hpp>
#include <shard/semantic/symbols/DelegateTypeSymbol.hpp>

#include <cstdint>
#include <cstddef>
#include <cstdio>
#include <vector>

namespace shard
{
	class SHARD_API ByteCodeDecoder
	{
		const std::vector<std::byte>& _code;
		std::size_t _ip = 0;

	public:
		inline ByteCodeDecoder(const std::vector<std::byte>& code) : _code(code) { }

		bool IsEOF();
		std::size_t Index() const;
		void SetCursor(std::int64_t amount);
		void Return();

		OpCode AbsorbOpCode();

		bool AbsorbBoolean();
		std::int64_t AbsorbInt64();
		double AbsorbDouble64();
		wchar_t AbsorbChar16();
		std::size_t AbsorbString();

		std::uint8_t AbsorbUInt8();
		std::uint16_t AbsorbVariableSlot();
		std::uint16_t AbsorbUInt16();
		std::size_t AbsorbJump();
		
		TypeSymbol* AbsorbTypeSymbol();
		DelegateTypeSymbol* AbsordDelegateTypeSymbol();
		FieldSymbol* AbsorbFieldSymbol();
		ArrayTypeSymbol* AbsorbArraySymbol();
		MethodSymbol* AbsorbMethodSymbol();
		ConstructorSymbol* AbsorbConstructorSymbol();
		MethodSymbolDelegate AbsorbFunctionPtr();
	};
}