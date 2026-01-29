#include <shard/framework/FrameworkModule.h>

#include <shard/runtime/InboundVariablesContext.h>
#include <shard/runtime/GarbageCollector.h>
#include <shard/runtime/ObjectInstance.h>

#include <shard/syntax/SymbolFactory.h>
#include <shard/syntax/symbols/TypeSymbol.h>
#include <shard/syntax/symbols/MethodSymbol.h>
#include <shard/syntax/symbols/FieldSymbol.h>
#include <shard/syntax/symbols/ClassSymbol.h>
#include <shard/syntax/symbols/ArrayTypeSymbol.h>
#include <shard/syntax/symbols/GenericTypeSymbol.h>

#include <shard/parsing/reading/SourceReader.h>
#include <shard/parsing/reading/StringStreamReader.h>
#include <shard/parsing/semantic/SymbolTable.h>

#include <Windows.h>
#include <string>
#include <stdexcept>
#include <malloc.h>

#include "../resources.h"

using namespace shard;

namespace shard
{
	class Collections_List : public FrameworkModule
	{
		// public extern List();
		static ObjectInstance* Impl_Default_Ctor(const MethodSymbol* symbol, InboundVariablesContext* arguments)
		{
			static FieldSymbol* arrayField = static_cast<ClassSymbol*>(symbol->Parent)->Fields.at(0); // T[] _array

			// Getting arguments
			ObjectInstance* instance = arguments->Variables.at(L"this"); // List<T> this

			// Initializing fields
			TypeSymbol* arrayType = TypeSymbol::ReturnOf(arrayField);
			ObjectInstance* arrayInstance = GarbageCollector::AllocateInstance(arrayType);
			instance->SetField(arrayField, arrayInstance);

			return nullptr; // void
		}

		// public static extern void List.Add(T value);
		static ObjectInstance* Impl_Add(const MethodSymbol* symbol, InboundVariablesContext* arguments)
		{
			static FieldSymbol* arrayField = static_cast<ClassSymbol*>(symbol->Parent)->Fields.at(0); // T[] _array
			static FieldSymbol* arrayLengthField = SymbolTable::Primitives::Array->Fields.at(0); // <Length>k__BackingField

			// Getting arguments
			ObjectInstance* instance = arguments->Variables.at(L"this"); // List<T> this
			ObjectInstance* value = arguments->Variables.at(L"value"); // T value

			// Getting fields
			ObjectInstance* arrayInstance = instance->GetField(arrayField); // T[] _array
			ArrayTypeSymbol* arrayType = static_cast<ArrayTypeSymbol*>(const_cast<TypeSymbol*>(arrayInstance->Info));

			// Modifing array
			size_t newSize = arrayType->MemoryBytesSize + value->Info->GetInlineSize();
			void* newPtr = realloc(arrayInstance->Ptr, newSize);

			if (newPtr == nullptr)
			{
				throw std::runtime_error("failed to resize List inner _array");
			}

			arrayType->Size += 1;
			arrayType->MemoryBytesSize = newSize;
			arrayInstance->Ptr = newPtr;
			
			// Adding element
			size_t indexLast = arrayType->Size - 1;
			ObjectInstance* valueCopy = GarbageCollector::CopyInstance(value);
			arrayInstance->SetElement(indexLast, valueCopy);

			// setting new array length
			int64_t arraySize = static_cast<int64_t>(arrayType->Size);
			arrayInstance->WriteMemory(0, sizeof(int64_t), &arraySize);

			return nullptr; // void
		}

		// public static extern T ElementAt(int index);
		static ObjectInstance* Impl_ElementAt(const MethodSymbol* symbol, InboundVariablesContext* arguments)
		{
			static FieldSymbol* arrayField = static_cast<ClassSymbol*>(symbol->Parent)->Fields.at(0); // T[] _array

			// Getting arguments
			ObjectInstance* instance = arguments->Variables.at(L"this"); // List<T> this
			ObjectInstance* index = arguments->Variables.at(L"index"); // int index

			ObjectInstance* arrayInstance = instance->GetField(arrayField); // T[] _array
			ArrayTypeSymbol* arrayType = static_cast<ArrayTypeSymbol*>(const_cast<TypeSymbol*>(arrayInstance->Info));

			// Getting value
			int64_t indexValue = index->AsInteger();
			if (!arrayInstance->IsInBounds(indexValue))
				throw std::runtime_error("index is out of bounds");

			return arrayInstance->GetElement(indexValue);
		}

		// public extern void RemoveAt(int index);
		static ObjectInstance* Impl_RemoveAt(const MethodSymbol* symbol, InboundVariablesContext* arguments)
		{
			throw std::runtime_error("not implemented");
		}

		// public extern void Clear();
		static ObjectInstance* Impl_Clear(const MethodSymbol* symbol, InboundVariablesContext* arguments)
		{
			throw std::runtime_error("not implemented");
		}

		// public int Length { extern get; }
		static ObjectInstance* Impl_Length_Get(const MethodSymbol* symbol, InboundVariablesContext* arguments) // public int Length { extern get; }
		{
			static FieldSymbol* arrayField = static_cast<ClassSymbol*>(symbol->Parent->Parent)->Fields.at(0); // T[] _array
			static FieldSymbol* arrayLengthField = SymbolTable::Primitives::Array->Fields.at(0); // <Length>k__BackingField

			// Getting arguments
			ObjectInstance* instance = arguments->Variables.at(L"this"); // List<T> this

			// Getting fields
			ObjectInstance* arrayInstance = instance->GetField(arrayField); // T[] _array
			ObjectInstance* lengthInstance = arrayInstance->GetField(arrayLengthField); // int length

			return lengthInstance;
		}

		static ObjectInstance* Impl_index_Get(const MethodSymbol* symbol, InboundVariablesContext* arguments)
		{
			static FieldSymbol* arrayField = static_cast<ClassSymbol*>(symbol->Parent->Parent)->Fields.at(0); // T[] _array

			// Getting arguments
			ObjectInstance* instance = arguments->Variables.at(L"this"); // List<T> this
			ObjectInstance* index = arguments->Variables.at(L"index"); // int index

			ObjectInstance* arrayInstance = instance->GetField(arrayField); // T[] _array
			ArrayTypeSymbol* arrayType = static_cast<ArrayTypeSymbol*>(const_cast<TypeSymbol*>(arrayInstance->Info));

			// Getting value
			int64_t indexValue = index->AsInteger();
			if (!arrayInstance->IsInBounds(indexValue))
				throw std::runtime_error("index is out of bounds");

			return arrayInstance->GetElement(indexValue);
		}

		static ObjectInstance* Impl_index_Set(const MethodSymbol* symbol, InboundVariablesContext* arguments)
		{
			static FieldSymbol* arrayField = static_cast<ClassSymbol*>(symbol->Parent->Parent)->Fields.at(0); // T[] _array

			// getting arguments
			ObjectInstance* instance = arguments->Variables.at(L"this"); // List<T> this
			ObjectInstance* index = arguments->Variables.at(L"index"); // int index
			ObjectInstance* value = arguments->Variables.at(L"value"); // T value

			ObjectInstance* arrayInstance = instance->GetField(arrayField); // T[] _array
			ArrayTypeSymbol* arrayType = static_cast<ArrayTypeSymbol*>(const_cast<TypeSymbol*>(arrayInstance->Info));

			// Getting value
			int64_t indexValue = index->AsInteger();
			if (!arrayInstance->IsInBounds(indexValue))
				throw std::runtime_error("index is out of bounds");

			arrayInstance->SetElement(indexValue, value);
			return nullptr;
		}

	public:
		SourceReader* FrameworkModule::GetSource()
		{
			const wchar_t* resourceData; size_t resourceSize;
			resources::GetResource(L"COLLECTIONS_LIST", resourceData, resourceSize);
			return new StringStreamReader(L"List.ss", resourceData, resourceSize / sizeof(wchar_t));
		}

		bool FrameworkModule::BindConstructor(ConstructorSymbol* symbol)
		{
			if (symbol->Parameters.size() == 0)
			{
				symbol->FunctionPointer = Impl_Default_Ctor;
				return true;
			}

			return false;
		}

		bool FrameworkModule::BindMethod(MethodSymbol* symbol)
		{
			if (symbol->Name == L"Add")
			{
				symbol->FunctionPointer = Impl_Add;
				return true;
			}

			if (symbol->Name == L"ElementAt")
			{
				symbol->FunctionPointer = Impl_ElementAt;
				return true;
			}

			if (symbol->Name == L"RemoveAt")
			{
				symbol->FunctionPointer = Impl_RemoveAt;
				return true;
			}

			if (symbol->Name == L"Clear")
			{
				symbol->FunctionPointer = Impl_Clear;
				return true;
			}

			return false;
		}

		bool FrameworkModule::BindAccessor(AccessorSymbol* symbol)
		{
			if (symbol->Name == L"Length_get")
			{
				symbol->FunctionPointer = Impl_Length_Get;
				return true;
			}

			if (symbol->Name == L"index_get")
			{
				symbol->FunctionPointer = Impl_index_Get;
				return true;
			}

			if (symbol->Name == L"index_set")
			{
				symbol->FunctionPointer = Impl_index_Set;
				return true;
			}

			return false;
		}
	};
}
