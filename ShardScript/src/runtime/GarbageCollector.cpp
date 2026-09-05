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

    static inline ObjectInstance::GcHeader* HeaderOf(std::byte* payload)
    {
        if (payload == nullptr)
            return nullptr;

        return reinterpret_cast<ObjectInstance::GcHeader*>(payload - sizeof(ObjectInstance::GcHeader));
    }
}

GarbageCollector::GarbageCollector(ApplicationDomain* domain) : applicationDomain(domain)
{
}

TypeShapeCache& GarbageCollector::GetTypeShapeCache() const
{
	if (applicationDomain->GetProgram().TypeShapes == nullptr)
		throw std::runtime_error("TypeShapeCache is not initialized");

	return *applicationDomain->GetProgram().TypeShapes;
}

ObjectInstance GarbageCollector::RecoverStaticField(FieldSymbol* field, std::byte* payload)
{
	TypeSymbol* type = field->ReturnType;
	TypeShape* shape = GetTypeShapeCache().GetOrCreateShape(type);
	return ObjectInstance(type, shape, payload);
}

ObjectInstance GarbageCollector::RecoverInternedString(std::byte* payload)
{
	TypeSymbol* type = SymbolTable::Primitives::String;
	TypeShape* shape = GetTypeShapeCache().GetOrCreateShape(type);
	return ObjectInstance(type, shape, payload);
}

ObjectInstance GarbageCollector::FromBoolean(bool value)
{
	TypeShape* shape = GetTypeShapeCache().GetOrCreateShape(SymbolTable::Primitives::Boolean);
	ObjectInstance instance = GarbageCollector::AllocateInstance(shape);
	instance.WriteBoolean(value);
	return instance;
}

ObjectInstance GarbageCollector::FromInteger(std::int64_t value)
{
	TypeShape* shape = GetTypeShapeCache().GetOrCreateShape(SymbolTable::Primitives::Integer);
	ObjectInstance instance = GarbageCollector::AllocateInstance(shape);
	instance.WriteInteger(value);
	return instance;
}

ObjectInstance GarbageCollector::FromByte(std::uint8_t value)
{
	TypeShape* shape = GetTypeShapeCache().GetOrCreateShape(SymbolTable::Primitives::Byte);
	ObjectInstance instance = GarbageCollector::AllocateInstance(shape);
	instance.WriteByte(value);
	return instance;
}

ObjectInstance GarbageCollector::FromDouble(double value)
{
	TypeShape* shape = GetTypeShapeCache().GetOrCreateShape(SymbolTable::Primitives::Double);
	ObjectInstance instance = GarbageCollector::AllocateInstance(shape);
	instance.WriteDouble(value);
	return instance;
}

ObjectInstance GarbageCollector::FromChar(wchar_t value)
{
	TypeShape* shape = GetTypeShapeCache().GetOrCreateShape(SymbolTable::Primitives::Char);
	ObjectInstance instance = GarbageCollector::AllocateInstance(shape);
	instance.WriteCharacter(value);
	return instance;
}

ObjectInstance GarbageCollector::FromString(const wchar_t* value)
{
	TypeShape* shape = GetTypeShapeCache().GetOrCreateShape(SymbolTable::Primitives::String);
	ObjectInstance instance = GarbageCollector::AllocateInstance(shape);

	std::size_t length = wcslen(value);
	std::uint64_t length64 = static_cast<std::uint64_t>(length);
	instance.WriteMemory(0, sizeof(std::int64_t), &length64);

	std::size_t size = (length + 1) * sizeof(wchar_t);
	wchar_t* copy = static_cast<wchar_t*>(AllocateBytes(size));

	if (copy == nullptr)
		throw std::runtime_error("Failed to allocate string");

	std::memcpy(copy, value, size);
	instance.WriteMemory(sizeof(std::int64_t), sizeof(wchar_t*), &copy);

	return instance;
}

ObjectInstance GarbageCollector::FromString(const std::wstring& value)
{
	TypeShape* shape = GetTypeShapeCache().GetOrCreateShape(SymbolTable::Primitives::String);
	ObjectInstance instance = GarbageCollector::AllocateInstance(shape);

	std::size_t length = value.size();
	std::size_t size = (length + 1) * sizeof(wchar_t);

	wchar_t* copy = static_cast<wchar_t*>(AllocateBytes(size));
	if (copy == nullptr)
		throw std::runtime_error("Failed to allocate string");

	std::memcpy(copy, value.c_str(), size);

	std::uint64_t length64 = static_cast<std::uint64_t>(length);
	instance.WriteMemory(0, sizeof(std::int64_t), &length64);
	instance.WriteMemory(sizeof(std::int64_t), sizeof(wchar_t*), &copy);
	return instance;
}

ObjectInstance GarbageCollector::FromNint(std::intptr_t rawMemory)
{
	return FromNint(reinterpret_cast<void*>(rawMemory));
}

ObjectInstance GarbageCollector::FromNint(std::uintptr_t rawMemory)
{
	return FromNint(reinterpret_cast<void*>(rawMemory));
}

ObjectInstance GarbageCollector::FromNint(void* rawMemory)
{
	TypeSymbol* objectInfo = SymbolTable::Primitives::NativeInteger;
	if (objectInfo == nullptr)
		throw std::runtime_error("objectInfo is nullptr");

	if (objectInfo->LayoutingState != TypeLayoutingState::Visited)
		throw std::runtime_error("objectInfo is uninitialized");

	TypeShape* shape = GetTypeShapeCache().GetOrCreateShape(objectInfo);
	ObjectInstance instance = AllocateInstance(shape);
	if (shape->Size > 0)
		instance.WriteMemory(0, shape->Size, &rawMemory);

	return instance;
}

ObjectInstance GarbageCollector::GetStaticField(FieldSymbol* field)
{
	if (auto find = staticFields.find(field); find != staticFields.end())
		return RecoverStaticField(field, find->second);

	if (field->DefaultValueExpression != nullptr)
	{
		// TODO: FIX!!! (evaluate the default-value expression)
		return ObjectInstance();
	}

	TypeSymbol* fieldType = field->ReturnType;
	TypeShape* shape = GetTypeShapeCache().GetOrCreateShape(fieldType);

	if (fieldType->Inlining == TypeInlining::ByValue)
	{
		ObjectInstance instance = AllocateInstance(shape);
		instance.IncrementReference(); // permanent root
		staticFields[field] = instance.getMemory();
		return instance;
	}

	staticFields[field] = nullptr;
	return ObjectInstance(fieldType, shape, nullptr);
}

void GarbageCollector::SetStaticField(FieldSymbol* field, ObjectInstance instance)
{
	if (instance.IsNullInstance())
		throw std::runtime_error("requested setting static field to nullptr");

	if (auto find = staticFields.find(field); find != staticFields.end())
	{
		ObjectInstance oldValue = RecoverStaticField(field, find->second);
		DestroyInstance(oldValue);
	}

	ObjectInstance stored = CopyInstance(instance);
	stored.IncrementReference();
	staticFields[field] = stored.getMemory();
}

ObjectInstance GarbageCollector::AllocateInstance(TypeShape* shape)
{
	if (shape == nullptr)
		throw std::runtime_error("shape is nullptr");

	const std::size_t prefixSize = sizeof(ObjectInstance::GcHeader);
	ObjectInstance::GcHeader* header = static_cast<ObjectInstance::GcHeader*>(AllocateZeroedBytes(prefixSize + shape->Size));
	if (header == nullptr)
		throw std::runtime_error("cannot allocate memory for new instance");

	header->Magic = ObjectInstance::GcHeader::MAGIC;
	header->Shape = shape;

	std::byte* payload = reinterpret_cast<std::byte*>(header) + sizeof(ObjectInstance::GcHeader);
	Heap.add(payload);
	return ObjectInstance(shape->BaseType, shape, payload);
}

ObjectInstance GarbageCollector::AllocateInstance(const TypeSymbol* objectInfo)
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

ObjectInstance GarbageCollector::AllocateGeneric(TypeSymbol* baseType, const std::span<TypeSymbol*> genericArgs)
{
	if (baseType == nullptr)
		throw std::runtime_error("baseType is nullptr");

	TypeShape* shape = GetTypeShapeCache().GetOrCreateShape(baseType, std::vector<TypeSymbol*>(genericArgs.begin(), genericArgs.end()));
	return AllocateInstance(shape);
}

ObjectInstance GarbageCollector::AllocateGeneric(TypeSymbol* baseType, const std::vector<TypeSymbol*>& genericArgs)
{
	if (baseType == nullptr)
		throw std::runtime_error("baseType is nullptr");

	TypeShape* shape = GetTypeShapeCache().GetOrCreateShape(baseType, genericArgs);
	return AllocateInstance(shape);
}

ObjectInstance GarbageCollector::AllocateArray(ArrayTypeSymbol* arrayType, TypeSymbol* elementType, std::size_t length)
{
	if (arrayType == nullptr)
		throw std::runtime_error("arrayType is nullptr");

	if (elementType == nullptr)
		throw std::runtime_error("elementType is nullptr");

	if (!IsTypeReadyForAllocation(elementType))
		throw std::runtime_error("Cannot allocate array of element type '" + WStringToUtf8(elementType->Name) + "': symbol is not ready for runtime");

	std::size_t headerSize = SymbolTable::Primitives::Array->MemoryBytesSize;
	std::size_t elementSize = elementType->GetInlineSize();
	std::size_t totalSize = headerSize + elementSize * length;

	const std::size_t prefixSize = sizeof(ObjectInstance::GcHeader);
	ObjectInstance::GcHeader* header = static_cast<ObjectInstance::GcHeader*>(AllocateZeroedBytes(prefixSize + totalSize));
	if (header == nullptr)
		throw std::runtime_error("cannot allocate memory for dynamic array");

	header->Magic = ObjectInstance::GcHeader::MAGIC;
	TypeShape* arrayShape = GetTypeShapeCache().GetOrCreateShape(arrayType);
	header->Shape = arrayShape;

	std::byte* payload = reinterpret_cast<std::byte*>(header) + sizeof(ObjectInstance::GcHeader);
	std::uint64_t payloadLength = static_cast<std::uint64_t>(length);
	std::memcpy(payload, &payloadLength, sizeof(payloadLength));

	Heap.add(payload);
	return ObjectInstance(arrayType, arrayShape, payload);
}

ObjectInstance GarbageCollector::CopyInstance(ObjectInstance instance)
{
	if (instance.IsNullInstance())
		return instance;

	const TypeSymbol* info = instance.getInfo();
	if (info != nullptr && info->IsReferenceType())
	{
		instance.IncrementReference();
		return instance;
	}

	TypeShape* shape = instance.getShape();
	ObjectInstance newInstance = shape != nullptr
		? AllocateInstance(shape)
		: AllocateInstance(info);

	TypeShape* newShape = newInstance.getShape();
	if (newShape == nullptr)
		throw std::runtime_error("cannot copy instance without a type shape");

	newInstance.WriteMemory(0, newShape->Size, instance.getMemory());

	for (std::uint32_t slot = 0; slot < static_cast<std::uint32_t>(newShape->Slots.size()); ++slot)
	{
		TypeShape* fieldShape = newShape->GetFieldShape(slot);
		if (fieldShape != nullptr && fieldShape->IsReferenceType())
		{
			ObjectInstance fieldValue = newInstance.GetField(slot);
			if (!fieldValue.IsNullInstance())
				fieldValue.IncrementReference();
		}
	}

	return newInstance;
}

bool GarbageCollector::IsHeapBacked(ObjectInstance instance)
{
	return instance.isHeapBacked();
}

void GarbageCollector::CollectInstance(ObjectInstance instance)
{
	if (!instance.isHeapBacked())
		return;

	if (instance.getReferencesCounter() > 0)
		return;

	Heap.erase(instance.getMemory());
	TerminateInstance(instance);
}

void GarbageCollector::DestroyInstance(ObjectInstance instance)
{
	if (!instance.isHeapBacked())
		return;

	ObjectInstance::GcHeader* header = instance.getGcHeader();
	if (header->Terminated)
		return;

	instance.DecrementReference();
	if (instance.getReferencesCounter() > 0)
		return;

	Heap.erase(instance.getMemory());
	TerminateInstance(instance);
}

void GarbageCollector::DeleteInstanceMemory(ObjectInstance instance)
{
	if (!instance.isHeapBacked())
		return;

	if (instance.getInfo() == SymbolTable::Primitives::String)
	{
		void* stringPtr = instance.OffsetMemory(sizeof(std::int64_t), sizeof(wchar_t*));
		wchar_t* stringData = *static_cast<wchar_t**>(stringPtr);
		FreeBytes(stringData);
	}

	if (ObjectInstance::GcHeader* header = instance.getGcHeader(); header != nullptr)
		FreeBytes(header);
}

void GarbageCollector::TerminateInstance(ObjectInstance instance, bool deleteInstance)
{
	if (!instance.isHeapBacked())
		return;

	ObjectInstance::GcHeader* header = instance.getGcHeader();
	if (header->Terminated)
		return;

	header->Terminated = true;

	asyncTable.erase(instance.getMemory());
	delegateTargets.erase(instance.getMemory());

	TypeShape* shape = instance.getShape();
	if (shape != nullptr)
	{
		for (std::uint32_t slot = 0; slot < static_cast<std::uint32_t>(shape->Slots.size()); ++slot)
		{
			TypeShape* fieldShape = shape->GetFieldShape(slot);
			if (fieldShape != nullptr && fieldShape->IsReferenceType())
			{
				ObjectInstance fieldValue = instance.GetField(slot);
				if (!fieldValue.IsNullInstance())
					DestroyInstance(fieldValue);
			}
		}
	}

	if (instance.getInfo()->Kind == SyntaxKind::ArrayType)
	{
		const ArrayTypeSymbol* arrayInfo = static_cast<const ArrayTypeSymbol*>(instance.getInfo());
		for (std::size_t i = 0; i < instance.GetArrayLength(); i++)
		{
			ObjectInstance element = instance.GetElement(i);
			if (arrayInfo->UnderlayingType->IsReferenceType() && !element.IsNullInstance())
				DestroyInstance(element);
		}
	}

	if (deleteInstance)
		DeleteInstanceMemory(instance);
}

void GarbageCollector::Terminate()
{
	// Snapshot all regular instances and mark them as terminating up front so
	// nested DestroyInstance calls never touch memory about to be freed.
	std::vector<std::byte*> snapshot;
	snapshot.reserve(Heap.size());

	for (std::byte* payload : Heap)
	{
		ObjectInstance::GcHeader* header = HeaderOf(payload);
		if (header != nullptr)
			header->Terminated = true;

		snapshot.push_back(payload);
	}

	// Destroy all static instances first.
	for (const auto& choice : staticFields)
	{
		std::byte* payload = choice.second;
		if (payload == nullptr)
			continue;

		ObjectInstance instance = RecoverStaticField(choice.first, payload);
		Heap.erase(payload);
		TerminateInstance(instance);
	}

	// Release references between regular instances without deleting them yet.
	for (std::byte* payload : snapshot)
	{
		ObjectInstance::GcHeader* header = HeaderOf(payload);
		if (header == nullptr || header->Shape == nullptr)
			continue;

		ObjectInstance instance(header->Shape->BaseType, header->Shape, payload);
		TerminateInstance(instance, false);
	}

	// Now that no regular instance holds a reference to another, delete them all.
	for (std::byte* payload : snapshot)
	{
		Heap.erase(payload);
		ObjectInstance::GcHeader* header = HeaderOf(payload);
		if (header != nullptr)
			FreeBytes(header);
	}

	Heap.clear();
	staticFields.clear();
	asyncTable.clear();
	delegateTargets.clear();
}

// Immortal instance: [GcHeader(zeroed magic)][payload] in one block. The magic
// is intentionally left unset so refcount/GC ops are no-ops, but the payload is
// a valid non-null pointer (so the value is never treated as null).
ObjectInstance GarbageCollector::CreateView(const TypeSymbol* info, TypeShape* shape)
{
	const std::size_t payloadBytes = shape != nullptr && shape->Size > 0 ? shape->Size : 1;
	ObjectInstance::GcHeader* header = static_cast<ObjectInstance::GcHeader*>(AllocateZeroedBytes(sizeof(ObjectInstance::GcHeader) + payloadBytes));

	std::byte* payload = reinterpret_cast<std::byte*>(header) + sizeof(ObjectInstance::GcHeader);
	return ObjectInstance(info, shape, payload);
}

ObjectInstance GarbageCollector::InternString(const wchar_t* value)
{
	if (auto find = internedStrings.find(value); find != internedStrings.end())
		return RecoverInternedString(find->second);

	TypeShape* shape = GetTypeShapeCache().GetOrCreateShape(SymbolTable::Primitives::String);
	ObjectInstance view = CreateView(shape->BaseType, shape);

	std::size_t length = wcslen(value);
	std::uint64_t length64 = static_cast<std::uint64_t>(length);
	view.WriteMemory(0, sizeof(std::int64_t), &length64);
	view.WriteMemory(sizeof(std::int64_t), sizeof(wchar_t*), &value);

	internedStrings.emplace(value, view.getMemory());
	return view;
}

bool GarbageCollector::IsTaskLike(ObjectInstance instance)
{
	if (auto find = asyncTable.find(instance.getMemory()); find != asyncTable.end())
		return find->second.IsTaskLike;

	return false;
}

void GarbageCollector::MarkTaskLike(ObjectInstance instance)
{
	asyncTable[instance.getMemory()].IsTaskLike = true;
}

bool GarbageCollector::IsFireAndForget(ObjectInstance instance)
{
	if (auto find = asyncTable.find(instance.getMemory()); find != asyncTable.end())
		return find->second.IsFireAndForget;

	return false;
}

void GarbageCollector::MarkFireAndForget(ObjectInstance instance)
{
	asyncTable[instance.getMemory()].IsFireAndForget = true;
}

void* GarbageCollector::GetAsyncNativeState(ObjectInstance instance)
{
	if (auto find = asyncTable.find(instance.getMemory()); find != asyncTable.end())
		return find->second.NativeState;

	return nullptr;
}

void GarbageCollector::SetAsyncNativeState(ObjectInstance instance, void* state)
{
	asyncTable[instance.getMemory()].NativeState = state;
}

std::shared_ptr<CallStackFrame> GarbageCollector::GetFrameOwner(ObjectInstance instance)
{
	if (auto find = asyncTable.find(instance.getMemory()); find != asyncTable.end())
		return find->second.FrameOwner;

	return nullptr;
}

void GarbageCollector::BindToFrame(ObjectInstance instance, std::shared_ptr<CallStackFrame> frame)
{
	std::byte* key = instance.getMemory();
	if (key == nullptr || frame == nullptr)
		return;

	AsyncRecord& record = asyncTable[key];
	if (record.FrameOwner == frame)
		return;

	ReleaseFrameOwner(instance);
	record.FrameOwner = std::move(frame);
	record.FrameOwner->PendingTaskCount++;
}

void GarbageCollector::ReleaseFrameOwner(ObjectInstance instance)
{
	if (auto find = asyncTable.find(instance.getMemory()); find != asyncTable.end())
	{
		if (find->second.FrameOwner != nullptr && find->second.FrameOwner->PendingTaskCount > 0)
			find->second.FrameOwner->PendingTaskCount--;

		find->second.FrameOwner.reset();
	}
}
