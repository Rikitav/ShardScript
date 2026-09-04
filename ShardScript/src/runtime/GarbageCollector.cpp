#include <shard/runtime/GarbageCollector.hpp>
#include <shard/runtime/ObjectInstance.hpp>
#include <shard/runtime/CallStackFrame.hpp>
#include <shard/runtime/Allocator.hpp>
#include <shard/TypeLayout.hpp>

#include <shard/parsing/SyntaxKind.hpp>

#include <shard/semantic/symbols/TypeSymbol.hpp>
#include <shard/semantic/symbols/FieldSymbol.hpp>
#include <shard/semantic/symbols/ArrayTypeSymbol.hpp>
#include <shard/semantic/symbols/GenericTypeSymbol.hpp>

#include <shard/semantic/SymbolTable.hpp>

#include <shard/compilation/ProgramVirtualImage.hpp>
#include <shard/ApplicationDomain.hpp>

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

alignas(std::uint64_t) static std::byte NullInstanceStorage[sizeof(ObjectInstance::GcHeader) + sizeof(ObjectInstance)] = {};
ObjectInstance* GarbageCollector::NullInstance = new (NullInstanceStorage + sizeof(ObjectInstance::GcHeader)) ObjectInstance(nullptr, nullptr, nullptr);

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

ObjectInstance* GarbageCollector::FromValue(const wchar_t* value)
{
	TypeShape* shape = GetTypeShapeCache().GetOrCreateShape(SymbolTable::Primitives::String);
	ObjectInstance* instance = GarbageCollector::AllocateInstance(shape);

	std::size_t length = wcslen(value);
	std::uint64_t length64 = static_cast<std::uint64_t>(length);
	instance->WriteMemory(0, sizeof(std::int64_t), &length64);

	std::size_t size = (length + 1) * sizeof(wchar_t);
	wchar_t* copy = static_cast<wchar_t*>(AllocateBytes(size));

	if (copy == nullptr)
		throw std::runtime_error("Failed to allocate string");

	std::memcpy(copy, value, size);
	instance->WriteMemory(sizeof(std::int64_t), sizeof(wchar_t*), &copy);

	return instance;
}

ObjectInstance* GarbageCollector::FromValue(const std::wstring& value)
{
	TypeShape* shape = GetTypeShapeCache().GetOrCreateShape(SymbolTable::Primitives::String);
	ObjectInstance* instance = GarbageCollector::AllocateInstance(shape);

	std::size_t length = value.size();
	std::size_t size = (length + 1) * sizeof(wchar_t);

	wchar_t* copy = static_cast<wchar_t*>(AllocateBytes(size));
	if (copy == nullptr)
		throw std::runtime_error("Failed to allocate string");

	std::memcpy(copy, value.c_str(), size);

	std::uint64_t length64 = static_cast<std::uint64_t>(length);
	instance->WriteMemory(0, sizeof(std::int64_t), &length64);
	instance->WriteMemory(sizeof(std::int64_t), sizeof(wchar_t*), &copy);
	return instance;
}

ObjectInstance* GarbageCollector::FromNint(std::intptr_t rawMemory)
{
	return FromNint(reinterpret_cast<void*>(rawMemory));
}

ObjectInstance* GarbageCollector::FromNint(std::uintptr_t rawMemory)
{
	return FromNint(reinterpret_cast<void*>(rawMemory));
}

ObjectInstance* GarbageCollector::FromNint(void* rawMemory)
{
	TypeSymbol* objectInfo = SymbolTable::Primitives::NativeInteger;
	if (objectInfo == nullptr)
		throw std::runtime_error("objectInfo is nullptr");

	if (objectInfo->LayoutingState != TypeLayoutingState::Visited)
		throw std::runtime_error("objectInfo is uninitialized");

	TypeShape* shape = GetTypeShapeCache().GetOrCreateShape(objectInfo);
	ObjectInstance* instance = AllocateInstance(shape);
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

	// Static field values are permanent roots: take the root reference up front
	// so eval-stack drains can never collect them out from under staticFields.
	staticFieldInstance->IncrementReference();
	staticFieldInstance->IsStaticRoot = true;

	staticFields[field] = staticFieldInstance;
	return staticFieldInstance;
}

void GarbageCollector::SetStaticField(FieldSymbol* field, ObjectInstance* instance)
{
	if (instance == nullptr)
		throw std::runtime_error("requested setting static field to nullptr");

	if (auto find = staticFields.find(field); find != staticFields.end())
	{
		// Unroot the old value, then release the static's reference to it.
		// (The old code called CollectInstance, which can never free a value
		// the static still references — leaking every replaced static.)
		ObjectInstance* oldValue = find->second;
		oldValue->IsStaticRoot = false;
		DestroyInstance(oldValue);
	}

	ObjectInstance* stored = CopyInstance(instance);
	stored->IncrementReference();
	stored->IsStaticRoot = true;
	staticFields[field] = stored;
}

ObjectInstance* GarbageCollector::AllocateInstance(TypeShape* shape)
{
	if (shape == nullptr)
		throw std::runtime_error("shape is nullptr");

	constexpr std::size_t prefixSize = sizeof(ObjectInstance::GcHeader) + sizeof(ObjectInstance);
	ObjectInstance::GcHeader* header = static_cast<ObjectInstance::GcHeader*>(AllocateZeroedBytes(prefixSize + shape->Size));
	if (header == nullptr)
		throw std::runtime_error("cannot allocate memory for new instance");

	header->Magic = ObjectInstance::GcHeader::MAGIC;
	auto* blockBytes = static_cast<std::byte*>(static_cast<void*>(header));
	void* rawMemory = blockBytes + prefixSize;

	ObjectInstance* instance = new (blockBytes + sizeof(ObjectInstance::GcHeader)) ObjectInstance(shape->BaseType, shape, rawMemory);
	Heap.add(instance);
	return instance;
}

ObjectInstance* GarbageCollector::AllocateInstance(const TypeSymbol* objectInfo)
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

		return AllocateGeneric(generic->UnderlayingType, genericArgs);
	}

	TypeShape* shape = GetTypeShapeCache().GetOrCreateShape(const_cast<TypeSymbol*>(objectInfo));
	return AllocateInstance(shape);
}

ObjectInstance* GarbageCollector::AllocateGeneric(TypeSymbol* baseType, const std::span<TypeSymbol*> genericArgs)
{
	if (baseType == nullptr)
		throw std::runtime_error("baseType is nullptr");

	TypeShape* shape = GetTypeShapeCache().GetOrCreateShape(baseType, std::vector<TypeSymbol*>(genericArgs.begin(), genericArgs.end()));
	return AllocateInstance(shape);
}

ObjectInstance* GarbageCollector::AllocateGeneric(TypeSymbol* baseType, const std::vector<TypeSymbol*>& genericArgs)
{
	if (baseType == nullptr)
		throw std::runtime_error("baseType is nullptr");

	TypeShape* shape = GetTypeShapeCache().GetOrCreateShape(baseType, genericArgs);
	return AllocateInstance(shape);
}

ObjectInstance* GarbageCollector::AllocateArray(TypeSymbol* elementType, std::size_t length)
{
	if (elementType == nullptr)
		throw std::runtime_error("elementType is nullptr");

	if (!IsTypeReadyForAllocation(elementType))
		throw std::runtime_error("Cannot allocate array of element type '" + WStringToUtf8(elementType->Name) + "': symbol is not ready for runtime");

	std::size_t headerSize = SymbolTable::Primitives::Array->MemoryBytesSize;
	std::size_t elementSize = elementType->GetInlineSize();
	std::size_t totalSize = headerSize + elementSize * length;

	constexpr std::size_t prefixSize = sizeof(ObjectInstance::GcHeader) + sizeof(ObjectInstance);
	ObjectInstance::GcHeader* header = static_cast<ObjectInstance::GcHeader*>(AllocateZeroedBytes(prefixSize + totalSize));
	if (header == nullptr)
		throw std::runtime_error("cannot allocate memory for dynamic array");

	header->Magic = ObjectInstance::GcHeader::MAGIC;
	auto* blockBytes = static_cast<std::byte*>(static_cast<void*>(header));
	void* rawMemory = blockBytes + prefixSize;

	ArrayTypeSymbol* arrayType = new ArrayTypeSymbol(elementType);
	arrayType->Length = length;
	arrayType->MemoryBytesSize = totalSize;
	dynamicArrayTypes.emplace_back(arrayType);

	TypeShape* arrayShape = new TypeShape(arrayType, std::vector<TypeSymbol*>{ elementType });
	arrayShape->Alignment = GetTypeAlignment(elementType);
	arrayShape->Size = totalSize;
	dynamicArrayShapes.emplace_back(arrayShape);

	ObjectInstance* instance = new (blockBytes + sizeof(ObjectInstance::GcHeader)) ObjectInstance(arrayType, arrayShape, rawMemory);
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
	if (!instance->isHeapBacked() && shape != nullptr && shape->IsReferenceType())
	{
		instance->IncrementReference();
		return instance;
	}

	ObjectInstance* newInstance = shape != nullptr
		? AllocateInstance(shape)
		: AllocateInstance(instance->getInfo());

	TypeShape* newShape = newInstance->getShape();
	if (newShape == nullptr)
		throw std::runtime_error("cannot copy instance without a type shape");

	newInstance->WriteMemory(0, newShape->Size, instance->getMemory());

	for (std::uint32_t slot = 0; slot < static_cast<std::uint32_t>(newShape->Slots.size()); ++slot)
	{
		TypeShape* fieldShape = newShape->GetFieldShape(slot);
		if (fieldShape != nullptr && fieldShape->IsReferenceType())
		{
			ObjectInstance fieldStorage(nullptr, nullptr, nullptr);
			ObjectInstance* fieldValue = newInstance->GetField(slot, fieldStorage);
			if (fieldValue != nullptr && fieldValue != NullInstance)
				fieldValue->IncrementReference();
		}
	}

	return newInstance;
}

bool GarbageCollector::IsHeapBacked(ObjectInstance* instance)
{
	return instance != nullptr && instance->isHeapBacked();
}

ObjectInstance* GarbageCollector::Materialize(ObjectInstance* value)
{
	if (value == nullptr || value == NullInstance || !value->IsView)
		return value;

	return CopyInstance(value);
}

void GarbageCollector::CollectInstance(ObjectInstance* instance)
{
	if (instance == nullptr || !instance->isHeapBacked())
		return;

	if (instance->getReferencesCounter() > 0)
		return;

	Heap.erase(instance);
	TerminateInstance(instance);
}

void GarbageCollector::DestroyInstance(ObjectInstance* instance)
{
	if (instance == nullptr || !instance->isHeapBacked())
		return;

	ObjectInstance::GcHeader* header = instance->getGcHeader();
	if (header->Terminated)
		return;

	instance->DecrementReference();
	if (instance->getReferencesCounter() > 0)
		return;

	Heap.erase(instance);
	TerminateInstance(instance);
}

void GarbageCollector::DeleteInstanceMemory(ObjectInstance* instance)
{
	if (instance == nullptr)
		throw std::runtime_error("requested deleting nullptr");

	if (!instance->isHeapBacked())
		return;

	if (instance->getInfo() == SymbolTable::Primitives::String)
	{
		void* stringPtr = instance->OffsetMemory(sizeof(std::int64_t), sizeof(wchar_t*));
		wchar_t* stringData = *static_cast<wchar_t**>(stringPtr);
		FreeBytes(stringData);
	}

	if (ObjectInstance::GcHeader* header = instance->getGcHeader(); header != nullptr)
		FreeBytes(header);
}

void GarbageCollector::TerminateInstance(ObjectInstance* instance, bool deleteInstance)
{
	if (instance == nullptr)
		throw std::runtime_error("requested terminating nullptr");

	if (!instance->isHeapBacked())
		return;

	ObjectInstance::GcHeader* header = instance->getGcHeader();
	if (header->Terminated)
		return;

	header->Terminated = true;

	asyncTable.erase(instance);

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

	if (deleteInstance)
		DeleteInstanceMemory(instance);
}

void GarbageCollector::Terminate()
{
	// Snapshot all regular instances and mark them as terminating up front.
	// This prevents use-after-free when static instances or other regular instances reference each other and are destroyed in an arbitrary order.
	std::vector<ObjectInstance*> snapshot;
	snapshot.reserve(Heap.size());

	for (ObjectInstance* instance : Heap)
	{
		if (ObjectInstance::GcHeader* header = instance->getGcHeader(); header != nullptr)
			header->Terminated = true;

		snapshot.push_back(instance);
	}

	// Destroy all static instances. They may reference regular instances, but
	// those are already marked terminating, so DestroyInstance will skip them.
	for (const auto& choise : staticFields)
	{
		ObjectInstance* instance = choise.second;
		Heap.erase(instance);
		TerminateInstance(instance);
	}

	// Release references between regular instances without deleting any of them yet.
	// Because every regular instance is already marked Terminated, nested
	// DestroyInstance calls will not touch memory that is about to be freed.
	for (ObjectInstance* instance : snapshot)
		TerminateInstance(instance, false);

	// Now that no regular instance holds a reference to another, delete them all.
	for (ObjectInstance* instance : snapshot)
	{
		Heap.erase(instance);
		DeleteInstanceMemory(instance);
	}

	Heap.clear();
	staticFields.clear();
	asyncTable.clear();
}

ObjectInstance* GarbageCollector::CreateView(const TypeSymbol* info, TypeShape* shape, void* memory)
{
	ObjectInstance::GcHeader* header = static_cast<ObjectInstance::GcHeader*>(AllocateZeroedBytes(sizeof(ObjectInstance::GcHeader) + sizeof(ObjectInstance)));
	return new (reinterpret_cast<std::byte*>(header) + sizeof(ObjectInstance::GcHeader)) ObjectInstance(info, shape, memory);
}

ObjectInstance* GarbageCollector::InternString(const wchar_t* value)
{
	if (auto find = internedStrings.find(value); find != internedStrings.end())
		return find->second;

	TypeShape* shape = GetTypeShapeCache().GetOrCreateShape(SymbolTable::Primitives::String);
	ObjectInstance* view = CreateView(shape->BaseType, shape, nullptr);

	std::size_t length = wcslen(value);
	std::uint64_t length64 = static_cast<std::uint64_t>(length);
	view->WriteMemory(0, sizeof(std::int64_t), &length64);
	view->WriteMemory(sizeof(std::int64_t), sizeof(wchar_t*), &value);

	internedStrings.emplace(value, view);
	return view;
}

bool GarbageCollector::IsTaskLike(ObjectInstance* instance)
{
	if (auto find = asyncTable.find(instance); find != asyncTable.end())
		return find->second.IsTaskLike;

	return false;
}

void GarbageCollector::MarkTaskLike(ObjectInstance* instance)
{
	asyncTable[instance].IsTaskLike = true;
}

bool GarbageCollector::IsFireAndForget(ObjectInstance* instance)
{
	if (auto find = asyncTable.find(instance); find != asyncTable.end())
		return find->second.IsFireAndForget;

	return false;
}

void GarbageCollector::MarkFireAndForget(ObjectInstance* instance)
{
	asyncTable[instance].IsFireAndForget = true;
}

void* GarbageCollector::GetAsyncNativeState(ObjectInstance* instance)
{
	if (auto find = asyncTable.find(instance); find != asyncTable.end())
		return find->second.NativeState;

	return nullptr;
}

void GarbageCollector::SetAsyncNativeState(ObjectInstance* instance, void* state)
{
	asyncTable[instance].NativeState = state;
}

std::shared_ptr<CallStackFrame> GarbageCollector::GetFrameOwner(ObjectInstance* instance)
{
	if (auto find = asyncTable.find(instance); find != asyncTable.end())
		return find->second.FrameOwner;

	return nullptr;
}

void GarbageCollector::BindToFrame(ObjectInstance* instance, std::shared_ptr<CallStackFrame> frame)
{
	if (instance == nullptr || frame == nullptr)
		return;

	AsyncRecord& record = asyncTable[instance];
	if (record.FrameOwner == frame)
		return;

	ReleaseFrameOwner(instance);
	record.FrameOwner = std::move(frame);
	record.FrameOwner->PendingTaskCount++;
}

void GarbageCollector::ReleaseFrameOwner(ObjectInstance* instance)
{
	if (auto find = asyncTable.find(instance); find != asyncTable.end())
	{
		if (find->second.FrameOwner != nullptr && find->second.FrameOwner->PendingTaskCount > 0)
			find->second.FrameOwner->PendingTaskCount--;

		find->second.FrameOwner.reset();
	}
}
