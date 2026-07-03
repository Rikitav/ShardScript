#include <shard/runtime/GarbageCollector.hpp>
#include <shard/runtime/ObjectInstance.hpp>

#include <shard/parsing/SyntaxKind.hpp>

#include <shard/semantic/symbols/TypeSymbol.hpp>
#include <shard/semantic/symbols/FieldSymbol.hpp>
#include <shard/semantic/symbols/ArrayTypeSymbol.hpp>
#include <shard/semantic/symbols/GenericTypeSymbol.hpp>

#include <shard/semantic/SymbolTable.hpp>

#include <malloc.h>
#include <stdexcept>
#include <cstring>
#include <wchar.h>
#include <cstdint>
#include <string>

using namespace shard;

namespace
{
    static inline std::string WStringToUtf8(const std::wstring& wstr)
    {
        std::string result;
        result.reserve(wstr.size());
        for (wchar_t ch : wstr)
            result.push_back(static_cast<char>(ch));
        return result;
    }

    static inline bool IsTypeReadyForAllocation(const TypeSymbol* type)
    {
        if (type == nullptr)
            return false;

        if (type->IsReadyForRuntime())
            return true;

        // Runtime-generated array types are synthetic and do not go through
        // the normal analysis pipeline.
        if (type->Kind == SyntaxKind::ArrayType)
            return true;

        // Generic instantiations inherit readiness from their underlying type.
        if (type->Kind == SyntaxKind::GenericType)
        {
            const GenericTypeSymbol* generic = static_cast<const GenericTypeSymbol*>(type);
            return IsTypeReadyForAllocation(generic->UnderlayingType);
        }

        return false;
    }
}

ObjectInstance* GarbageCollector::NullInstance = new ObjectInstance(nullptr, nullptr, true);

GarbageCollector::GarbageCollector(ApplicationDomain* domain) : applicationDomain(domain)
{

}

ObjectInstance* GarbageCollector::FromValue(bool value)
{
	ObjectInstance* instance = GarbageCollector::AllocateInstance(SymbolTable::Primitives::Boolean);
	instance->WriteBoolean(value);
	return instance;
}

ObjectInstance* GarbageCollector::FromValue(std::int64_t value)
{
	ObjectInstance* instance = GarbageCollector::AllocateInstance(SymbolTable::Primitives::Integer);
	instance->WriteInteger(value);
	return instance;
}

ObjectInstance* GarbageCollector::FromValue(double value)
{
	ObjectInstance* instance = GarbageCollector::AllocateInstance(SymbolTable::Primitives::Double);
	instance->WriteDouble(value);
	return instance;
}

ObjectInstance* GarbageCollector::FromValue(wchar_t value)
{
	ObjectInstance* instance = GarbageCollector::AllocateInstance(SymbolTable::Primitives::Char);
	instance->WriteCharacter(value);
	return instance;
}

/*
ObjectInstance* GarbageCollector::FromValue(const char* value)
{
	ObjectInstance* instance = GarbageCollector::AllocateInstance(SymbolTable::Primitives::String, false);

	std::size_t length = strlen(value);
	wchar_t* wvalue = new wchar_t[length];
	mbstowcs(wvalue, value, length + 1);

	instance->WriteMemory(0, sizeof(std::int64_t), static_cast<std::uint64_t*>(&length));
	instance->WriteMemory(sizeof(std::int64_t), sizeof(wchar_t*), &wvalue);
	return instance;
}
*/

ObjectInstance* GarbageCollector::FromValue(const wchar_t* value, bool isTransient)
{
	ObjectInstance* instance = GarbageCollector::AllocateInstance(SymbolTable::Primitives::String, isTransient);

	std::size_t length = wcslen(value);
	instance->WriteMemory(0, sizeof(std::int64_t), static_cast<std::uint64_t*>(&length));
	instance->WriteMemory(sizeof(std::int64_t), sizeof(wchar_t*), &value);
	return instance;
}

ObjectInstance* GarbageCollector::FromValue(const std::wstring& value)
{
	ObjectInstance* instance = GarbageCollector::AllocateInstance(SymbolTable::Primitives::String, false);

	std::size_t length = value.size();
	std::size_t size = (length + 1) * sizeof(wchar_t);

	wchar_t* copy = static_cast<wchar_t*>(malloc(size));
	if (copy == nullptr)
		throw std::runtime_error("Failed to allocate string");

	std::memcpy(copy, value.c_str(), size);

	instance->WriteMemory(0, sizeof(std::int64_t), static_cast<std::uint64_t*>(&length));
	instance->WriteMemory(sizeof(std::int64_t), sizeof(wchar_t*), &copy);
	return instance;
}

ObjectInstance* GarbageCollector::FromNint(void* rawMemory, bool isTransient)
{
	TypeSymbol* objectInfo = SymbolTable::Primitives::NativeInteger;
	if (objectInfo == nullptr)
		throw std::runtime_error("objectInfo is nullptr");

	if (objectInfo->LayoutingState != TypeLayoutingState::Visited)
		throw std::runtime_error("objectInfo is uninitialized");

	ObjectInstance* instance = AllocateInstance(objectInfo, isTransient);
	if (objectInfo->MemoryBytesSize > 0)
		instance->WriteMemory(0, objectInfo->MemoryBytesSize, &rawMemory);

	return instance;
}

ObjectInstance* GarbageCollector::GetStaticField(FieldSymbol* field)
{
	if (auto find = staticFields.find(field); find != staticFields.end())
		return find->second;

	if (field->DefaultValueExpression != nullptr)
	{
		// TODO: FIX!!!
		/*
		ObjectInstance* staticFieldInstance = AbstractInterpreter::EvaluateExpression(field->DefaultValueExpression);
		staticFields[field] = staticFieldInstance;
		return staticFieldInstance;
		*/
		return nullptr;
	}

	ObjectInstance* staticFieldInstance = field->ReturnType->Inlining == TypeInlining::ByValue
		? GarbageCollector::AllocateInstance(field->ReturnType)
		: NullInstance;

	staticFields[field] = staticFieldInstance;
	return staticFieldInstance;
}

void GarbageCollector::SetStaticField(FieldSymbol* field, ObjectInstance* instance)
{
	if (instance == nullptr)
		throw std::runtime_error("requested setting static field to nullptr");

	if (auto find = staticFields.find(field); find != staticFields.end())
	{
		ObjectInstance* oldValue = GetStaticField(field);
		GarbageCollector::CollectInstance(oldValue);
	}

	staticFields[field] = CopyInstance(instance);
}

ObjectInstance* GarbageCollector::AllocateInstance(const TypeSymbol* objectInfo, bool isTransient)
{
	if (objectInfo == nullptr)
		throw std::runtime_error("objectInfo is nullptr");

	if (objectInfo->LayoutingState != TypeLayoutingState::Visited)
		throw std::runtime_error("objectInfo is uninitialized");

	if (!IsTypeReadyForAllocation(objectInfo))
		throw std::runtime_error("Cannot allocate instance of type '" + WStringToUtf8(objectInfo->Name) + "': symbol is not ready for runtime");

	void* rawMemory = nullptr;
	if (objectInfo->MemoryBytesSize > 0)
	{
		rawMemory = malloc(objectInfo->MemoryBytesSize);
		if (rawMemory == nullptr)
			throw std::runtime_error("cannot allocate memory for new instance");

		std::memset(rawMemory, 0, objectInfo->MemoryBytesSize);
	}

	ObjectInstance* instance = new ObjectInstance(objectInfo, rawMemory, isTransient);
	if (objectInfo->Kind == SyntaxKind::ArrayType)
	{
		const ArrayTypeSymbol* arrayInfo = static_cast<const ArrayTypeSymbol*>(objectInfo);
		instance->SetArrayLength(arrayInfo->Length);
		instance->SetMemorySize(objectInfo->MemoryBytesSize);
	}

	Heap.add(instance);
	return instance;
}

ObjectInstance* GarbageCollector::AllocateArray(TypeSymbol* elementType, std::size_t length, bool isTransient)
{
	if (elementType == nullptr)
		throw std::runtime_error("elementType is nullptr");

	if (!IsTypeReadyForAllocation(elementType))
		throw std::runtime_error("Cannot allocate array of element type '" + WStringToUtf8(elementType->Name) + "': symbol is not ready for runtime");

	std::size_t headerSize = SymbolTable::Primitives::Array->MemoryBytesSize;
	std::size_t elementSize = elementType->GetInlineSize();
	std::size_t totalSize = headerSize + elementSize * length;

	void* rawMemory = nullptr;
	if (totalSize > 0)
	{
		rawMemory = malloc(totalSize);
		if (rawMemory == nullptr)
			throw std::runtime_error("cannot allocate memory for dynamic array");

		std::memset(rawMemory, 0, totalSize);
	}

	ArrayTypeSymbol* arrayType = new ArrayTypeSymbol(elementType);
	arrayType->Length = length;
	arrayType->MemoryBytesSize = totalSize;
	dynamicArrayTypes.emplace_back(arrayType);

	ObjectInstance* instance = new ObjectInstance(arrayType, rawMemory, isTransient);
	instance->SetArrayLength(length);
	instance->SetMemorySize(totalSize);
	Heap.add(instance);
	return instance;
}

ObjectInstance* GarbageCollector::CopyInstance(ObjectInstance* instance)
{
	if (instance == nullptr)
		throw std::runtime_error("requested copying nullptr");

	if (instance == NullInstance)
		return instance;

	if (instance->getInfo()->Inlining == TypeInlining::ByReference)
	{
		instance->IncrementReference();
		return instance;
	}

	ObjectInstance* newInstance = GarbageCollector::AllocateInstance(instance->getInfo());
	newInstance->WriteMemory(0, instance->getInfo()->MemoryBytesSize, instance->getMemory());

	TypeSymbol* fieldOwner = const_cast<TypeSymbol*>(instance->getInfo());
	if (fieldOwner->Kind == SyntaxKind::GenericType)
		fieldOwner = static_cast<GenericTypeSymbol*>(fieldOwner)->UnderlayingType;

	for (FieldSymbol* field : fieldOwner->Fields)
	{
		if (field->ReturnType->Inlining == TypeInlining::ByReference)
		{
			ObjectInstance* fieldValue = instance->GetField(field);
			fieldValue->IncrementReference();
		}
	}

	return newInstance;
}

void GarbageCollector::CollectInstance(ObjectInstance* instance)
{
	if (instance == nullptr)
		throw std::runtime_error("requested destroying nullptr");

	if (instance == NullInstance)
		return;

	if (instance->getReferencesCounter() > 0)
		return;

	Heap.erase(instance);
	TerminateInstance(instance);
}

void GarbageCollector::DestroyInstance(ObjectInstance* instance)
{
	if (instance == nullptr)
		throw std::runtime_error("requested destroying nullptr");

	if (instance == NullInstance)
		return;

	instance->DecrementReference();
	if (instance->getReferencesCounter() > 0)
		return;

	Heap.erase(instance);
	TerminateInstance(instance);
}

void GarbageCollector::TerminateInstance(ObjectInstance* instance)
{
	if (instance == nullptr)
		throw std::runtime_error("requested terminating nullptr");

	if (instance == NullInstance)
		return;

	if (instance->Terminated)
		return;

	instance->Terminated = true;

	TypeSymbol* fieldOwner = const_cast<TypeSymbol*>(instance->getInfo());
	if (fieldOwner->Kind == SyntaxKind::GenericType)
		fieldOwner = static_cast<GenericTypeSymbol*>(fieldOwner)->UnderlayingType;

	for (FieldSymbol* field : fieldOwner->Fields)
	{
		if (field->ReturnType->Inlining == TypeInlining::ByReference)
		{
			ObjectInstance* fieldValue = instance->GetField(field);
			if (fieldValue != nullptr)
				DestroyInstance(fieldValue);
		}
	}

	if (instance->getInfo()->Kind == SyntaxKind::ArrayType)
	{
		const ArrayTypeSymbol* array = static_cast<const ArrayTypeSymbol*>(instance->getInfo());
		for (std::size_t i = 0; i < array->Length; i++)
		{
			ObjectInstance* element = instance->GetElement(i);
			if (element != nullptr)
				DestroyInstance(element);
		}
	}

	if (!instance->getIsTransient())
	{
		if (instance->getInfo() == SymbolTable::Primitives::String)
		{
			void* stringPtr = instance->OffsetMemory(sizeof(std::int64_t), sizeof(wchar_t*));
			wchar_t* stringData = *static_cast<wchar_t**>(stringPtr);
			free(stringData);
		}

		free(instance->getMemory());
	}

	delete instance;
}

void GarbageCollector::Terminate()
{
	// Destroy all static instances
	for (const auto& choise : staticFields)
		TerminateInstance(choise.second);
	staticFields.clear();

	// Snapshot and clear heap before terminating to avoid iterator invalidation
	std::vector<ObjectInstance*> snapshot;
	snapshot.reserve(Heap.size());
	for (ObjectInstance* instance : Heap)
		snapshot.push_back(instance);
	Heap.clear();

	// Destroy all regular instances
	for (ObjectInstance* instance : snapshot)
		TerminateInstance(instance);
}
