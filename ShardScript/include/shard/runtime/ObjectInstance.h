#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/syntax/symbols/TypeSymbol.h>
#include <shard/syntax/symbols/FieldSymbol.h>

#include <string>
#include <cstdint>

namespace shard
{
	class SHARD_API ObjectInstance
	{
	public:
		const TypeSymbol* Info;
		void* const Memory;
		size_t ReferencesCounter;
		const bool IsTransient;

		inline ObjectInstance(const TypeSymbol* info, void* memory, bool isTransient)
			: Info(info), Memory(memory), IsTransient(isTransient), ReferencesCounter(0) { }
		
		inline ~ObjectInstance() = default;

		static ObjectInstance* FromValue(int64_t value);
		static ObjectInstance* FromValue(double value);
		static ObjectInstance* FromValue(bool value);
		static ObjectInstance* FromValue(wchar_t value);
		static ObjectInstance* FromValue(const wchar_t* value, bool isTransient);
		static ObjectInstance* FromValue(const std::wstring& value);

		ObjectInstance* GetField(FieldSymbol* field);
		void SetField(FieldSymbol* field, ObjectInstance* instance);

		ObjectInstance* GetElement(size_t index);
		void SetElement(size_t index, ObjectInstance* instance);
		bool IsInBounds(size_t index);

		void IncrementReference();
		void DecrementReference();

		void WriteBoolean(const bool& value) const;
		void WriteInteger(const int64_t& value) const;
		void WriteDouble(const double& value) const;
		void WriteCharacter(const wchar_t& value) const;
		void WriteString(const wchar_t* value) const;
		void WriteString(const wchar_t* value, size_t size) const;
		void WriteString(const std::wstring& value) const;

		bool& AsBoolean() const;
		int64_t& AsInteger() const;
		double& AsDouble() const;
		wchar_t& AsCharacter() const;
		const wchar_t* AsString() const;

		void* GetObjectMemory() const;
		void* OffsetMemory(const size_t offset, const size_t size) const;
		void ReadMemory(const size_t offset, const size_t size, void* dst) const;
		void WriteMemory(const size_t offset, const size_t size, const void* src) const;
	};
}
