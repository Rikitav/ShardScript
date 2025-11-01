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
		const void* Ptr;

		ObjectInstance(const long id, const shard::syntax::symbols::TypeSymbol* info, const void* ptr)
			: Id(id), Info(info), Ptr(ptr) { }

		ObjectInstance(const ObjectInstance& other)
			: Id(other.Id), Info(other.Info), Ptr(other.Ptr) { }

		~ObjectInstance() = default;

		ObjectInstance* Copy();
		ObjectInstance* GetField(shard::syntax::symbols::FieldSymbol* field);
		void SetField(shard::syntax::symbols::FieldSymbol* field, ObjectInstance* instance);
		void CopyTo(ObjectInstance* to);
		unsigned long GetReferencesCount();
		void IncrementReference();
		void DecrementReference();

		template<typename T>
		inline void WritePrimitive(T value)
		{
			if (sizeof(T) != Info->MemoryBytesSize)
				throw std::runtime_error("size of primitive is not equal to instance size");

			WriteMemory(0, Info->MemoryBytesSize, &value);
		}

		template<typename T>
		inline T ReadPrimitive()
		{
			return *reinterpret_cast<T*>((void*)Ptr);
		}

	private:
		void* OffsetMemory(const size_t offset, const size_t size);
		void ReadMemory(const size_t offset, const size_t size, const void* dst);
		void WriteMemory(const size_t offset, const size_t size, const void* src);
	};
}
