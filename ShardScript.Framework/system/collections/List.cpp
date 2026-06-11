#include <ShardScript.hpp>
#include <stdexcept>
#include <string>

using namespace shard;

namespace shard
{
	extern "C"
	{
		__declspec(dllexport) ObjectInstance* shard_list_Ctor(const CallState& context) noexcept(false)
		{
			ObjectInstance* listInstance = context.Args[0];

			TypeSymbol* ownerType = const_cast<TypeSymbol*>(listInstance->getInfo());
			if (ownerType->Kind == SyntaxKind::GenericType)
				ownerType = static_cast<GenericTypeSymbol*>(ownerType)->UnderlayingType;

			FieldSymbol* arrayField = ownerType->Fields[0];
			TypeSymbol* concreteT = context.Frame->TypeArguments[0];

			ArrayTypeSymbol* arrayType = new ArrayTypeSymbol(concreteT);
			arrayType->Size = 0;
			arrayType->MemoryBytesSize = SymbolTable::Primitives::Array->MemoryBytesSize;
			arrayType->State = TypeLayoutingState::Visited;

			ObjectInstance* array = context.Collector.AllocateInstance(arrayType);
			listInstance->SetField(arrayField, array, context.Frame);

			return nullptr;
		}

		__declspec(dllexport) ObjectInstance* shard_list_Add(const CallState& context) noexcept(false)
		{
			ObjectInstance* listInstance = context.Args[0];
			ObjectInstance* value = context.Args[1];

			TypeSymbol* ownerType = const_cast<TypeSymbol*>(listInstance->getInfo());
			if (ownerType->Kind == SyntaxKind::GenericType)
				ownerType = static_cast<GenericTypeSymbol*>(ownerType)->UnderlayingType;

			FieldSymbol* arrayField = ownerType->Fields[0];
			ObjectInstance* arrayInstance = listInstance->GetField(arrayField, context.Frame);
			ArrayTypeSymbol* arrayType = static_cast<ArrayTypeSymbol*>(const_cast<TypeSymbol*>(arrayInstance->getInfo()));
			TypeSymbol* concreteT = context.Frame->TypeArguments[0];

			size_t currentSize = arrayType->Size;
			size_t newSize = currentSize + 1;

			ArrayTypeSymbol* newArrayType = new ArrayTypeSymbol(concreteT);
			newArrayType->Size = newSize;
			newArrayType->MemoryBytesSize = SymbolTable::Primitives::Array->MemoryBytesSize + concreteT->GetInlineSize() * newSize;
			newArrayType->State = TypeLayoutingState::Visited;

			ObjectInstance* newArray = context.Collector.AllocateInstance(newArrayType);

			for (size_t i = 0; i < currentSize; i++)
			{
				ObjectInstance* element = arrayInstance->GetElement(i, context.Frame);
				newArray->SetElement(i, element, context.Frame);
			}

			newArray->SetElement(currentSize, value, context.Frame);
			listInstance->SetField(arrayField, newArray, context.Frame);

			return nullptr;
		}

		__declspec(dllexport) ObjectInstance* shard_list_ElementAt(const CallState& context) noexcept(false)
		{
			ObjectInstance* listInstance = context.Args[0];
			int64_t index = context.Args[1]->AsInteger();

			TypeSymbol* ownerType = const_cast<TypeSymbol*>(listInstance->getInfo());
			if (ownerType->Kind == SyntaxKind::GenericType)
				ownerType = static_cast<GenericTypeSymbol*>(ownerType)->UnderlayingType;

			FieldSymbol* arrayField = ownerType->Fields[0];
			ObjectInstance* arrayInstance = listInstance->GetField(arrayField, context.Frame);
			ArrayTypeSymbol* arrayType = static_cast<ArrayTypeSymbol*>(const_cast<TypeSymbol*>(arrayInstance->getInfo()));

			if (index < 0 || static_cast<size_t>(index) >= arrayType->Size)
				throw std::runtime_error("index is out of bounds");

			return arrayInstance->GetElement(static_cast<size_t>(index), context.Frame);
		}

		__declspec(dllexport) ObjectInstance* shard_list_RemoveAt(const CallState& context) noexcept(false)
		{
			ObjectInstance* listInstance = context.Args[0];
			int64_t index = context.Args[1]->AsInteger();

			TypeSymbol* ownerType = const_cast<TypeSymbol*>(listInstance->getInfo());
			if (ownerType->Kind == SyntaxKind::GenericType)
				ownerType = static_cast<GenericTypeSymbol*>(ownerType)->UnderlayingType;

			FieldSymbol* arrayField = ownerType->Fields[0];
			ObjectInstance* arrayInstance = listInstance->GetField(arrayField, context.Frame);
			ArrayTypeSymbol* arrayType = static_cast<ArrayTypeSymbol*>(const_cast<TypeSymbol*>(arrayInstance->getInfo()));
			TypeSymbol* concreteT = context.Frame->TypeArguments[0];

			if (index < 0 || static_cast<size_t>(index) >= arrayType->Size)
				throw std::runtime_error("index is out of bounds");

			size_t newSize = arrayType->Size - 1;

			ArrayTypeSymbol* newArrayType = new ArrayTypeSymbol(concreteT);
			newArrayType->Size = newSize;
			newArrayType->MemoryBytesSize = SymbolTable::Primitives::Array->MemoryBytesSize + concreteT->GetInlineSize() * newSize;
			newArrayType->State = TypeLayoutingState::Visited;

			ObjectInstance* newArray = context.Collector.AllocateInstance(newArrayType);

			for (size_t i = 0; i < static_cast<size_t>(index); i++)
			{
				ObjectInstance* element = arrayInstance->GetElement(i, context.Frame);
				newArray->SetElement(i, element, context.Frame);
			}

			for (size_t i = static_cast<size_t>(index) + 1; i < arrayType->Size; i++)
			{
				ObjectInstance* element = arrayInstance->GetElement(i, context.Frame);
				newArray->SetElement(i - 1, element, context.Frame);
			}

			listInstance->SetField(arrayField, newArray, context.Frame);
			return nullptr;
		}

		__declspec(dllexport) ObjectInstance* shard_list_Clear(const CallState& context) noexcept(false)
		{
			ObjectInstance* listInstance = context.Args[0];

			TypeSymbol* ownerType = const_cast<TypeSymbol*>(listInstance->getInfo());
			if (ownerType->Kind == SyntaxKind::GenericType)
				ownerType = static_cast<GenericTypeSymbol*>(ownerType)->UnderlayingType;

			FieldSymbol* arrayField = ownerType->Fields[0];
			TypeSymbol* concreteT = context.Frame->TypeArguments[0];

			ArrayTypeSymbol* newArrayType = new ArrayTypeSymbol(concreteT);
			newArrayType->Size = 0;
			newArrayType->MemoryBytesSize = SymbolTable::Primitives::Array->MemoryBytesSize;
			newArrayType->State = TypeLayoutingState::Visited;

			ObjectInstance* newArray = context.Collector.AllocateInstance(newArrayType);
			listInstance->SetField(arrayField, newArray, context.Frame);

			return nullptr;
		}

		__declspec(dllexport) ObjectInstance* shard_list_Length_get(const CallState& context) noexcept(false)
		{
			ObjectInstance* listInstance = context.Args[0];

			TypeSymbol* ownerType = const_cast<TypeSymbol*>(listInstance->getInfo());
			if (ownerType->Kind == SyntaxKind::GenericType)
				ownerType = static_cast<GenericTypeSymbol*>(ownerType)->UnderlayingType;

			FieldSymbol* arrayField = ownerType->Fields[0];
			ObjectInstance* arrayInstance = listInstance->GetField(arrayField, context.Frame);
			ArrayTypeSymbol* arrayType = static_cast<ArrayTypeSymbol*>(const_cast<TypeSymbol*>(arrayInstance->getInfo()));

			return context.Collector.FromValue(static_cast<int64_t>(arrayType->Size));
		}

		__declspec(dllexport) ObjectInstance* shard_list_Indexer_get(const CallState& context) noexcept(false)
		{
			ObjectInstance* listInstance = context.Args[0];
			int64_t index = context.Args[1]->AsInteger();

			TypeSymbol* ownerType = const_cast<TypeSymbol*>(listInstance->getInfo());
			if (ownerType->Kind == SyntaxKind::GenericType)
				ownerType = static_cast<GenericTypeSymbol*>(ownerType)->UnderlayingType;

			FieldSymbol* arrayField = ownerType->Fields[0];
			ObjectInstance* arrayInstance = listInstance->GetField(arrayField, context.Frame);
			ArrayTypeSymbol* arrayType = static_cast<ArrayTypeSymbol*>(const_cast<TypeSymbol*>(arrayInstance->getInfo()));

			if (index < 0 || static_cast<size_t>(index) >= arrayType->Size)
				throw std::runtime_error("index is out of bounds");

			return arrayInstance->GetElement(static_cast<size_t>(index), context.Frame);
		}

		__declspec(dllexport) ObjectInstance* shard_list_Indexer_set(const CallState& context) noexcept(false)
		{
			ObjectInstance* listInstance = context.Args[0];
			int64_t index = context.Args[1]->AsInteger();
			ObjectInstance* value = context.Args[2];

			TypeSymbol* ownerType = const_cast<TypeSymbol*>(listInstance->getInfo());
			if (ownerType->Kind == SyntaxKind::GenericType)
				ownerType = static_cast<GenericTypeSymbol*>(ownerType)->UnderlayingType;

			FieldSymbol* arrayField = ownerType->Fields[0];
			ObjectInstance* arrayInstance = listInstance->GetField(arrayField, context.Frame);
			ArrayTypeSymbol* arrayType = static_cast<ArrayTypeSymbol*>(const_cast<TypeSymbol*>(arrayInstance->getInfo()));

			if (index < 0 || static_cast<size_t>(index) >= arrayType->Size)
				throw std::runtime_error("index is out of bounds");

			arrayInstance->SetElement(static_cast<size_t>(index), value, context.Frame);
			return nullptr;
		}
	}
}
