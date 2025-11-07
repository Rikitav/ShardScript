#pragma once
#include <shard/syntax/symbols/TypeSymbol.h>
#include <shard/syntax/symbols/FieldSymbol.h>
#include <stdexcept>
#include <string>

namespace shard::runtime
{
	class ObjectInstance
	{
	public:
		const long Id;
		const shard::syntax::symbols::TypeSymbol* Info;
		const bool IsNullable = false;
		size_t ReferencesCounter;
		void* Ptr;

		inline ObjectInstance(const long id, const shard::syntax::symbols::TypeSymbol* info, void* ptr)
			: Id(id), Info(info), Ptr(ptr), ReferencesCounter(0) { }
		
		inline ~ObjectInstance() = default;

		static ObjectInstance* FromValue(int value);
		static ObjectInstance* FromValue(bool value);
		static ObjectInstance* FromValue(wchar_t value);
		static ObjectInstance* FromValue(const wchar_t* value);
		static ObjectInstance* FromValue(const std::wstring& value);

		ObjectInstance* GetField(shard::syntax::symbols::FieldSymbol* field);
		void SetField(shard::syntax::symbols::FieldSymbol* field, ObjectInstance* instance);
		ObjectInstance* GetElement(size_t index);
		void SetElement(size_t index, ObjectInstance* instance);
		
		void IncrementReference();
		void DecrementReference();

		template<typename T>
		inline void WritePrimitive(T& value)
		{
			void* ptr = &value;
			WriteMemory(0, Info->MemoryBytesSize, ptr);
		}

		template<typename T>
		inline T ReadPrimitive()
		{
			return *reinterpret_cast<T*>(Ptr);
		}

		void* OffsetMemory(const size_t offset, const size_t size);
		void ReadMemory(const size_t offset, const size_t size, void* dst);
		void WriteMemory(const size_t offset, const size_t size, void* src);
	};
}
