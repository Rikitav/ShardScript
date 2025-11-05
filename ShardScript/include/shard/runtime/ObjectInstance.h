#pragma once
#include <shard/syntax/symbols/TypeSymbol.h>
#include <shard/syntax/symbols/FieldSymbol.h>
#include <stdexcept>

namespace shard::runtime
{
	class ObjectInstance
	{
	public:
		const long Id;
		const shard::syntax::symbols::TypeSymbol* Info;
		const bool IsNullable = false;
		void* Ptr;
		size_t ReferencesCounter;

		ObjectInstance(const long id, const shard::syntax::symbols::TypeSymbol* info, void* ptr)
			: Id(id), Info(info), Ptr(ptr), ReferencesCounter(0) { }

		/*
		ObjectInstance(const ObjectInstance& other)
			: Id(other.Id), Info(other.Info), Ptr(other.Ptr) { }
		*/

		~ObjectInstance() = default;

		ObjectInstance* GetField(shard::syntax::symbols::FieldSymbol* field);
		void SetField(shard::syntax::symbols::FieldSymbol* field, ObjectInstance* instance);
		
		void IncrementReference();
		void DecrementReference();

		template<typename T>
		inline void WritePrimitive(T& value)
		{
			WriteMemory(0, Info->MemoryBytesSize, &value);
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
