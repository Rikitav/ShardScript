#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/syntax/symbols/TypeSymbol.h>
#include <shard/syntax/symbols/FieldSymbol.h>

#include <stack>
#include <string>

namespace shard
{
	class SHARD_API ObjectInstance
	{
	public:
		const uint64_t Id;
		const shard::TypeSymbol* Info;
		const bool IsNullable = false;

		bool IsFieldInstance = false;
		size_t ReferencesCounter;
		void* Ptr;

		inline ObjectInstance(const uint64_t id, const shard::TypeSymbol* info, void* ptr)
			: Id(id), Info(info), Ptr(ptr), ReferencesCounter(0) { }
		
		inline ~ObjectInstance() = default;

		static ObjectInstance* FromValue(int64_t value);
		static ObjectInstance* FromValue(double value);
		static ObjectInstance* FromValue(bool value);
		static ObjectInstance* FromValue(wchar_t value);
		static ObjectInstance* FromValue(const wchar_t* value);
		static ObjectInstance* FromValue(const std::wstring& value);

		ObjectInstance* GetField(shard::FieldSymbol* field);
		void SetField(shard::FieldSymbol* field, ObjectInstance* instance);

		ObjectInstance* GetElement(size_t index);
		void SetElement(size_t index, ObjectInstance* instance);
		bool IsInBounds(size_t index);

		void IncrementReference();
		void DecrementReference();

		void WriteBoolean(const bool& value) const;
		void WriteInteger(const int64_t& value) const;
		void WriteDouble(const double& value) const;
		void WriteCharacter(const wchar_t& value) const;
		void WriteString(const std::wstring& value) const;

		bool AsBoolean() const;
		int64_t AsInteger() const;
		double AsDouble() const;
		wchar_t AsCharacter() const;
		std::wstring& AsString() const;

		void* OffsetMemory(const size_t offset, const size_t size) const;
		void ReadMemory(const size_t offset, const size_t size, void* dst) const;
		void WriteMemory(const size_t offset, const size_t size, const void* src) const;
	};
}
