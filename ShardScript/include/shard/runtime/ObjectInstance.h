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
		void* Ptr;
		size_t ReferencesCounter;

		ObjectInstance(const long id, const shard::syntax::symbols::TypeSymbol* info, void* ptr)
			: Id(id), Info(info), Ptr(ptr), ReferencesCounter(0) { }

		/*
		ObjectInstance(const ObjectInstance& other)
			: Id(other.Id), Info(other.Info), Ptr(other.Ptr) { }
		*/

		~ObjectInstance() = default;

		ObjectInstance* CopyReference();
		void CopyTo(ObjectInstance* to);

		const void* GetFieldMemory(shard::syntax::symbols::FieldSymbol* field);
		ObjectInstance* GetField(shard::syntax::symbols::FieldSymbol* field);
		void SetField(shard::syntax::symbols::FieldSymbol* field, ObjectInstance* instance);
		
		//unsigned long GetReferencesCount();
		void IncrementReference();
		void DecrementReference();

		template<typename T>
		inline void WritePrimitive(T& value)
		{
			WriteMemory(0, Info->MemoryBytesSize, &value);

			/*
			if (sizeof(T) != Info->MemoryBytesSize)
				throw std::runtime_error("size of primitive is not equal to instance size");

			if (Info->IsReferenceType)
			{
				T* primPtr = new T(value);
				WriteMemory(0, sizeof(T*), &primPtr);
			}
			else
			{
				WriteMemory(0, Info->MemoryBytesSize, &value);
			}
			*/
		}

		template<typename T>
		inline T ReadPrimitive()
		{
			return *reinterpret_cast<T*>(Ptr);

			/*
			void* primPtr = Ptr;
			if (Info->IsReferenceType)
				primPtr = *(static_cast<void**>(primPtr));
			
			return *reinterpret_cast<T*>(primPtr);
			*/
		}

	private:
		void* OffsetMemory(const size_t offset, const size_t size);
		void ReadMemory(const size_t offset, const size_t size, void* dst);
		void WriteMemory(const size_t offset, const size_t size, void* src);
	};
}
