#include <shard/runtime/GarbageCollector.hpp>
#include <shard/runtime/ObjectInstance.hpp>

#include <shard/parsing/SyntaxKind.hpp>

#include <shard/semantic/symbols/TypeSymbol.hpp>
#include <shard/semantic/symbols/FieldSymbol.hpp>
#include <shard/semantic/symbols/ArrayTypeSymbol.hpp>
#include <shard/semantic/symbols/GenericTypeSymbol.hpp>

#include <shard/semantic/SymbolTable.hpp>

#include <shard/ApplicationDomain.hpp>
#include <shard/compilation/ProgramVirtualImage.hpp>

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

        if (type->Kind == SyntaxKind::ArrayType)
            return true;

        if (type->Kind == SyntaxKind::GenericType)
        {
            const GenericTypeSymbol* generic = static_cast<const GenericTypeSymbol*>(type);
            return IsTypeReadyForAllocation(generic->UnderlayingType);
        }

        return false;
    }
}

ObjectInstance* GarbageCollector::NullInstance = new ObjectInstance(nullptr, nullptr, nullptr, true);

GarbageCollector::GarbageCollector(ApplicationDomain* domain) : applicationDomain(domain)
{

}

TypeShapeCache& GarbageCollector::GetTypeShapeCache() const
{
	if (applicationDomain->GetProgram().TypeShapes == nullptr)
		throw std::runtime_error("TypeShapeCache is not initialized");

	return *applicationDomain->GetProgram().TypeShapes;
}

ObjectInstance* GarbageCollector::FromValue(bool value)
{
	TypeShape* shape = GetTypeShapeCache().GetOrCreateShape(SymbolTable::Primitives::Boolean);
	ObjectInstance* instance = GarbageCollector::AllocateInstance(shape);
	instance->WriteBoolean(value);
	return instance;
}

ObjectInstance* GarbageCollector::FromValue(std::int64_t value)
{
	TypeShape* shape = GetTypeShapeCache().GetOrCreateShape(SymbolTable::Primitives::Integer);
	ObjectInstance* instance = GarbageCollector::AllocateInstance(shape);
	instance->WriteInteger(value);
	return instance;
}

ObjectInstance* GarbageCollector::FromValue(std::uint8_t value)
{
	TypeShape* shape = GetTypeShapeCache().GetOrCreateShape(SymbolTable::Primitives::Byte);
	ObjectInstance* instance = GarbageCollector::AllocateInstance(shape);
	instance->WriteByte(value);
	return instance;
}

ObjectInstance* GarbageCollector::FromValue(double value)
{
	TypeShape* shape = GetTypeShapeCache().GetOrCreateShape(SymbolTable::Primitives::Double);
	ObjectInstance* instance = GarbageCollector::AllocateInstance(shape);
	instance->WriteDouble(value);
	return instance;
}

ObjectInstance* GarbageCollector::FromValue(wchar_t value)
{
	TypeShape* shape = GetTypeShapeCache().GetOrCreateShape(SymbolTable::Primitives::Char);
	ObjectInstance* instance = GarbageCollector::AllocateInstance(shape);
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
	TypeShape* shape = GetTypeShapeCache().GetOrCreateShape(SymbolTable::Primitives::String);
	ObjectInstance* instance = GarbageCollector::AllocateInstance(shape, isTransient);

	std::size_t length = wcslen(value);
	instance->WriteMemory(0, sizeof(std::int64_t), static_cast<std::uint64_t*>(&length));
	instance->WriteMemory(sizeof(std::int64_t), sizeof(wchar_t*), &value);
	return instance;
}

ObjectInstance* GarbageCollector::FromValue(const std::wstring& value)
{
	TypeShape* shape = GetTypeShapeCache().GetOrCreateShape(SymbolTable::Primitives::String);
	ObjectInstance* instance = GarbageCollector::AllocateInstance(shape, false);

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

	TypeShape* shape = GetTypeShapeCache().GetOrCreateShape(objectInfo);
	ObjectInstance* instance = AllocateInstance(shape, isTransient);
	if (shape->Size > 0)
		instance->WriteMemory(0, shape->Size, &rawMemory);

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

	TypeSymbol* fieldType = field->ReturnType;
	ObjectInstance* staticFieldInstance = nullptr;
	if (fieldType->Inlining == TypeInlining::ByValue)
	{
		TypeShape* shape = GetTypeShapeCache().GetOrCreateShape(fieldType);
		staticFieldInstance = GarbageCollector::AllocateInstance(shape);
	}
	else
	{
		staticFieldInstance = NullInstance;
	}

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

ObjectInstance* GarbageCollector::AllocateInstance(TypeShape* shape, bool isTransient)
{
	if (shape == nullptr)
		throw std::runtime_error("shape is nullptr");

	void* rawMemory = nullptr;
	if (shape->Size > 0)
	{
		rawMemory = malloc(shape->Size);
		if (rawMemory == nullptr)
			throw std::runtime_error("cannot allocate memory for new instance");

		std::memset(rawMemory, 0, shape->Size);
	}

	ObjectInstance* instance = new ObjectInstance(shape->BaseType, shape, rawMemory, isTransient);
	Heap.add(instance);
	return instance;
}

ObjectInstance* GarbageCollector::AllocateInstance(const TypeSymbol* objectInfo, bool isTransient)
{
	if (objectInfo == nullptr)
		throw std::runtime_error("objectInfo is nullptr");

	if (!IsTypeReadyForAllocation(objectInfo))
		throw std::runtime_error("Cannot allocate instance of type '" + WStringToUtf8(objectInfo->Name) + "': symbol is not ready for runtime");

	if (objectInfo->Kind == SyntaxKind::GenericType)
	{
		GenericTypeSymbol* generic = const_cast<GenericTypeSymbol*>(static_cast<const GenericTypeSymbol*>(objectInfo));
		std::vector<TypeSymbol*> genericArgs;
		genericArgs.reserve(generic->UnderlayingType->TypeParameters.size());
		for (TypeParameterSymbol* parameter : generic->UnderlayingType->TypeParameters)
			genericArgs.push_back(generic->SubstituteTypeParameters(parameter));

		return AllocateGeneric(generic->UnderlayingType, genericArgs, isTransient);
	}

	TypeShape* shape = GetTypeShapeCache().GetOrCreateShape(const_cast<TypeSymbol*>(objectInfo));
	return AllocateInstance(shape, isTransient);
}

ObjectInstance* GarbageCollector::AllocateGeneric(TypeSymbol* baseType, const std::vector<TypeSymbol*>& genericArgs, bool isTransient)
{
	if (baseType == nullptr)
		throw std::runtime_error("baseType is nullptr");

	TypeShape* shape = GetTypeShapeCache().GetOrCreateShape(baseType, genericArgs);
	return AllocateInstance(shape, isTransient);
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

	TypeShape* arrayShape = new TypeShape(arrayType, std::vector<TypeSymbol*>{ elementType });
	arrayShape->Size = totalSize;
	dynamicArrayShapes.emplace_back(arrayShape);

	ObjectInstance* instance = new ObjectInstance(arrayType, arrayShape, rawMemory, isTransient);
	Heap.add(instance);
	return instance;
}

ObjectInstance* GarbageCollector::CopyInstance(ObjectInstance* instance)
{
	if (instance == nullptr)
		throw std::runtime_error("requested copying nullptr");

	if (instance == NullInstance)
		return instance;

	TypeShape* shape = instance->getShape();
	if (shape == nullptr)
		throw std::runtime_error("cannot copy instance without a type shape");

	if (shape->IsReferenceType())
	{
		instance->IncrementReference();
		return instance;
	}

	ObjectInstance* newInstance = GarbageCollector::AllocateInstance(shape);
	newInstance->WriteMemory(0, shape->Size, instance->getMemory());

	for (std::uint32_t slot = 0; slot < static_cast<std::uint32_t>(shape->Slots.size()); ++slot)
	{
		TypeShape* fieldShape = shape->GetFieldShape(slot);
		if (fieldShape != nullptr && fieldShape->IsReferenceType())
		{
			ObjectInstance* fieldValue = instance->GetField(slot);
			if (fieldValue != nullptr && fieldValue != NullInstance)
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

	TypeShape* shape = instance->getShape();
	if (shape != nullptr)
	{
		for (std::uint32_t slot = 0; slot < static_cast<std::uint32_t>(shape->Slots.size()); ++slot)
		{
			TypeShape* fieldShape = shape->GetFieldShape(slot);
			if (fieldShape != nullptr && fieldShape->IsReferenceType())
			{
				ObjectInstance* fieldValue = instance->GetField(slot);
				if (fieldValue != nullptr && fieldValue != NullInstance)
					DestroyInstance(fieldValue);
			}
		}
	}

	if (instance->getInfo()->Kind == SyntaxKind::ArrayType)
	{
		const ArrayTypeSymbol* array = static_cast<const ArrayTypeSymbol*>(instance->getInfo());
		for (std::size_t i = 0; i < array->Length; i++)
		{
			ObjectInstance* element = instance->GetElement(i);
			if (element != nullptr && element != NullInstance)
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


	// Snapshot and clear heap before terminating to avoid iterator invalidation
	std::vector<ObjectInstance*> snapshot;
	snapshot.reserve(Heap.size());
	
	for (ObjectInstance* instance : Heap)
		snapshot.push_back(instance);
	
	// Destroy all regular instances
	for (ObjectInstance* instance : snapshot)
		TerminateInstance(instance);

	Heap.clear();
	staticFields.clear();
}
