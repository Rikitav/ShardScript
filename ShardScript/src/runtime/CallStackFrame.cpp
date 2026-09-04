#include <shard/runtime/CallStackFrame.hpp>
#include <shard/runtime/VirtualMachine.hpp>
#include <shard/runtime/ObjectInstance.hpp>
#include <shard/runtime/GarbageCollector.hpp>
#include <shard/runtime/TypeShapeCache.hpp>

#include <shard/compilation/ProgramVirtualImage.hpp>

#include <shard/semantic/symbols/TypeParameterSymbol.hpp>
#include <shard/semantic/symbols/GenericTypeSymbol.hpp>
#include <shard/semantic/symbols/ParameterSymbol.hpp>

#include <mimalloc.h>

#include <algorithm>
#include <cstring>
#include <new>
#include <stdexcept>

using namespace shard;

namespace
{
	static void AdoptInlinePayload(TypeShape* shape, std::byte* payload)
	{
		if (shape == nullptr)
			return;

		for (std::uint32_t slot = 0; slot < static_cast<std::uint32_t>(shape->Slots.size()); ++slot)
		{
			TypeShape* fieldShape = shape->GetFieldShape(slot);
			if (fieldShape == nullptr || !fieldShape->IsReferenceType())
				continue;

			ObjectInstance* fieldValue;
			std::memcpy(&fieldValue, payload + shape->GetOffset(slot), sizeof(fieldValue));
			if (fieldValue != nullptr && fieldValue != GarbageCollector::NullInstance)
				fieldValue->IncrementReference();
		}
	}

	static void ReleaseInlinePayload(TypeShape* shape, std::byte* payload, GarbageCollector& gc)
	{
		if (shape == nullptr)
			return;

		for (std::uint32_t slot = 0; slot < static_cast<std::uint32_t>(shape->Slots.size()); ++slot)
		{
			TypeShape* fieldShape = shape->GetFieldShape(slot);
			if (fieldShape == nullptr || !fieldShape->IsReferenceType())
				continue;

			ObjectInstance* fieldValue;
			std::memcpy(&fieldValue, payload + shape->GetOffset(slot), sizeof(fieldValue));
			if (fieldValue != nullptr && fieldValue != GarbageCollector::NullInstance)
				gc.DestroyInstance(fieldValue);
		}
	}

	static void ReleaseBoxedPayload(ObjectInstance* value, GarbageCollector& gc)
	{
		if (value == nullptr)
			return;

		if (value->IsView)
			gc.DeleteView(value);
		else
			gc.DestroyInstance(value);
	}

	static void DiscardBoxedPayload(ObjectInstance* value, GarbageCollector& gc)
	{
		if (value == nullptr)
			return;

		if (value->IsView)
			gc.DeleteView(value);
		else
			gc.CollectInstance(value);
	}

	static TypeSymbol* ResolveSlotType(const MethodSymbol& method, std::uint16_t slot, const std::vector<TypeSymbol*>& typeArguments)
	{
		const std::uint16_t argsCount = method.GetEvalStackArgumentsCount();
		if (slot < argsCount)
		{
			std::size_t paramIndex = slot;
			if (method.Linking == LINK_INSTANCE)
			{
				if (slot == 0)
					return nullptr;

				--paramIndex;
			}

			if (paramIndex < method.Parameters.size() && method.Parameters[paramIndex] != nullptr)
				return const_cast<TypeSymbol*>(method.Parameters[paramIndex]->Type);

			return nullptr;
		}

		const std::size_t variableIndex = static_cast<std::size_t>(slot) - argsCount;
		if (variableIndex >= method.Layout.VariableSlots.size())
			return nullptr;

		const FrameSlotRecipe& recipe = method.Layout.VariableSlots[variableIndex];
		if (recipe.TypeParameterIndex >= 0)
		{
			if (static_cast<std::size_t>(recipe.TypeParameterIndex) < typeArguments.size())
				return typeArguments[recipe.TypeParameterIndex];

			return nullptr;
		}

		return recipe.ConcreteType;
	}

	static TypeShape* ResolveSlotShape(TypeSymbol* type, TypeShapeCache& shapes)
	{
		if (type == nullptr || type->IsReferenceType())
			return nullptr;

		std::vector<TypeSymbol*> genericArgs;
		TypeSymbol* baseType = type;
		if (type->Kind == SyntaxKind::GenericType)
		{
			GenericTypeSymbol* generic = static_cast<GenericTypeSymbol*>(type);
			baseType = generic->UnderlayingType;
			for (TypeParameterSymbol* parameter : baseType->TypeParameters)
				genericArgs.push_back(generic->SubstituteTypeParameters(parameter));
		}

		if (baseType == nullptr || baseType->IsReferenceType())
			return nullptr;

		return shapes.GetOrCreateShape(baseType, genericArgs);
	}
}

TypeSymbol* CallStackFrame::ResolveType(TypeSymbol* type)
{
	if (type == nullptr || type->Kind != SyntaxKind::TypeParameter)
		return type;

	TypeParameterSymbol* typeParam = static_cast<TypeParameterSymbol*>(type);
	std::uint16_t index = typeParam->TypeArgumentIndex;
	if (index < TypeArguments.size())
		return TypeArguments[index];

	return type;
}

std::shared_ptr<CallStackFrame> CallStackFrame::Create(const VirtualMachine* host, CallStackFrame* previousFrame, MethodSymbol* method, const std::vector<TypeSymbol*>& typeArguments)
{
	TypeShapeCache& shapes = *host->GetProgram().TypeShapes;

	const std::size_t localsCount = method->GetEvalStackLocalsCount();
	std::vector<LocalSlotDesc> slotDescs;
	slotDescs.reserve(localsCount);

	std::uint32_t offset = static_cast<std::uint32_t>(BoxedEntryStride);
	for (std::size_t slot = 0; slot < localsCount; slot++)
	{
		TypeShape* shape = ResolveSlotShape(ResolveSlotType(*method, static_cast<std::uint16_t>(slot), typeArguments), shapes);
		const bool isInline = shape != nullptr;
		const std::size_t stride = SlotHeaderBytes + (isInline ? Align(shape->Size) : ReferencePayloadBytes);

		slotDescs.push_back(LocalSlotDesc{ shape, offset, isInline });
		offset += static_cast<std::uint32_t>(stride);
	}

	const std::size_t localsBytes = offset - BoxedEntryStride;

	const std::size_t evalMaxPayload = std::max<std::size_t>(ReferencePayloadBytes, method->Layout.EvalSlotPayload);
	const std::size_t evalEntryStride = SlotHeaderBytes + Align(evalMaxPayload);
	const std::size_t evalEntries = method->Layout.IsComplete
		? std::max<std::size_t>(method->Layout.MaxEvalDepth, 8) : 64;
	const std::size_t evalCapacityBytes = evalEntries * (method->Layout.IsComplete ? evalEntryStride : BoxedEntryStride);

	const std::size_t arenaBytes = BoxedEntryStride + localsBytes + evalCapacityBytes;

	void* block = mi_malloc(sizeof(CallStackFrame) + arenaBytes);
	if (block == nullptr)
		throw std::runtime_error("Failed to allocate call stack frame");

	CallStackFrame* frame = new (block) CallStackFrame(host, previousFrame, method);

	std::shared_ptr<CallStackFrame> result(frame, [](CallStackFrame* ptr)
	{
		ptr->~CallStackFrame();
		mi_free(ptr);
	});

	frame->Arena = reinterpret_cast<std::byte*>(block) + sizeof(CallStackFrame);
	frame->ArenaBytes = arenaBytes;
	frame->ArenaIsTrailing = true;

	frame->ReturnSlot = frame->Arena;
	frame->LocalSlots = std::move(slotDescs);
	frame->LocalRegionEnd = frame->Arena + BoxedEntryStride + localsBytes;
	frame->EvalEntries = frame->LocalRegionEnd;
	frame->EvalCapacityBytes = evalCapacityBytes;
	frame->EvalMaxEntryStride = method->Layout.IsComplete ? evalEntryStride : BoxedEntryStride;
	frame->EvalOffsets.reserve(evalEntries);

	std::memset(frame->Arena, 0, BoxedEntryStride + localsBytes);
	return result;
}

ObjectInstance* CallStackFrame::WrapPayload(TypeShape* shape, std::byte* payload)
{
	ObjectInstance* view = new ObjectInstance(shape->BaseType, shape, payload, true);
	view->IsView = true;
	return view;
}

void CallStackFrame::GrowEvalRegion(std::size_t newCapacityBytes)
{
	if (newCapacityBytes <= EvalCapacityBytes)
		return;

	const std::size_t prefixBytes = static_cast<std::size_t>(LocalRegionEnd - Arena);

	if (ArenaIsTrailing)
	{
		std::byte* sideArena = static_cast<std::byte*>(mi_malloc(prefixBytes + newCapacityBytes));
		if (sideArena == nullptr)
			throw std::runtime_error("Failed to grow call stack frame arena");

		std::memcpy(sideArena, Arena, prefixBytes + EvalCursorBytes);

		Arena = sideArena;
		ArenaBytes = prefixBytes + newCapacityBytes;
		ArenaIsTrailing = false;
	}
	else
	{
		std::byte* newArena = static_cast<std::byte*>(mi_realloc(Arena, prefixBytes + newCapacityBytes));
		if (newArena == nullptr)
			throw std::runtime_error("Failed to grow call stack frame arena");

		Arena = newArena;
		ArenaBytes = prefixBytes + newCapacityBytes;
	}

	ReturnSlot = Arena;
	LocalRegionEnd = Arena + prefixBytes;
	EvalEntries = LocalRegionEnd;
	EvalCapacityBytes = newCapacityBytes;
}

std::byte* CallStackFrame::PushInlineUninitialized(TypeShape* shape)
{
	if (shape == nullptr)
		throw std::runtime_error("Cannot push an inline value without a type shape");

	const std::size_t stride = SlotHeaderBytes + Align(shape->Size);
	if (EvalCursorBytes + stride > EvalCapacityBytes)
		GrowEvalRegion(std::max(EvalCapacityBytes * 2, EvalCursorBytes + stride));

	std::byte* entry = EvalEntries + EvalCursorBytes;
	EvalOffsets.push_back(static_cast<std::uint32_t>(EvalCursorBytes));
	EvalCursorBytes += stride;
	EvalSize++;

	*reinterpret_cast<TypeShape**>(entry) = shape;
	std::memset(entry + SlotHeaderBytes, 0, stride - SlotHeaderBytes);
	return entry + SlotHeaderBytes;
}

void CallStackFrame::PushInline(TypeShape* shape, const void* payloadBytes)
{
	std::byte* payload = PushInlineUninitialized(shape);
	if (payloadBytes != nullptr)
		std::memcpy(payload, payloadBytes, shape->Size);

	AdoptInlinePayload(shape, payload);
}

void CallStackFrame::PushReference(ObjectInstance* value)
{
	if (EvalCursorBytes + BoxedEntryStride > EvalCapacityBytes)
		GrowEvalRegion(std::max(EvalCapacityBytes * 2, EvalCursorBytes + BoxedEntryStride));

	std::byte* entry = EvalEntries + EvalCursorBytes;
	EvalOffsets.push_back(static_cast<std::uint32_t>(EvalCursorBytes));
	EvalCursorBytes += BoxedEntryStride;
	EvalSize++;

	const std::uintptr_t header = reinterpret_cast<std::uintptr_t>(value != nullptr ? value->getShape() : nullptr) | BoxedTag;
	std::memcpy(entry, &header, sizeof(header));
	PayloadRef(entry) = value;
}

StackValue CallStackFrame::TopValue()
{
	if (EvalSize == 0)
		throw std::runtime_error("Evaluation stack underflow");

	const std::byte* entry = EvalEntries + EvalOffsets.back();
	return StackValue{ EntryShape(entry), !EntryIsBoxed(entry), const_cast<std::byte*>(entry) + SlotHeaderBytes };
}

StackValue CallStackFrame::PopValue()
{
	StackValue value = TopValue();
	EvalOffsets.pop_back();
	EvalCursorBytes -= EntryStride(value.Data - SlotHeaderBytes);
	EvalSize--;
	return value;
}

void CallStackFrame::PushCopy(const StackValue& value)
{
	if (value.IsInline)
		PushInline(value.Shape, value.Data);
	else
		PushReference(value.AsObject());
}

void CallStackFrame::ReleaseValue(const StackValue& value, GarbageCollector& gc)
{
	if (value.IsInline)
		ReleaseInlinePayload(value.Shape, value.Data, gc);
	else
		ReleaseBoxedPayload(value.AsObject(), gc);
}

void CallStackFrame::DiscardValue(const StackValue& value, GarbageCollector& gc)
{
	if (value.IsInline)
		ReleaseInlinePayload(value.Shape, value.Data, gc);
	else
		DiscardBoxedPayload(value.AsObject(), gc);
}

ObjectInstance* CallStackFrame::PopStack()
{
	StackValue value = PopValue();
	if (value.IsInline)
		throw std::runtime_error("Popped an inline eval entry as a reference");

	return value.AsObject();
}

ObjectInstance* CallStackFrame::PeekStack()
{
	StackValue value = TopValue();
	if (value.IsInline)
		throw std::runtime_error("Peeked an inline eval entry as a reference");

	return value.AsObject();
}

ObjectInstance* CallStackFrame::PopBoxed(GarbageCollector& gc)
{
	StackValue value = PopValue();
	if (!value.IsInline)
		return value.AsObject();

	ObjectInstance* box = gc.AllocateInstance(value.Shape);
	box->WriteMemory(0, value.Shape->Size, value.Data);
	return box;
}

ObjectInstance* CallStackFrame::GetLocal(std::uint16_t slot)
{
	const LocalSlotDesc& desc = LocalSlots.at(slot);
	std::byte* entry = Arena + desc.Offset;

	if (desc.Inline)
		return WrapPayload(desc.Shape, entry + SlotHeaderBytes);

	return PayloadRead(entry);
}

StackValue CallStackFrame::GetLocalValue(std::uint16_t slot)
{
	const LocalSlotDesc& desc = LocalSlots.at(slot);
	std::byte* entry = Arena + desc.Offset;

	if (desc.Inline)
		return StackValue{ desc.Shape, true, entry + SlotHeaderBytes };

	return StackValue{ nullptr, false, entry + SlotHeaderBytes };
}

ObjectInstance*& CallStackFrame::LocalRef(std::uint16_t slot)
{
	const LocalSlotDesc& desc = LocalSlots.at(slot);
	if (desc.Inline)
		throw std::runtime_error("Local slot holds an inline value, not a reference");

	return PayloadRef(Arena + desc.Offset);
}

void CallStackFrame::SetLocal(std::uint16_t slot, const StackValue& value, GarbageCollector& gc)
{
	const LocalSlotDesc& desc = LocalSlots.at(slot);
	std::byte* entry = Arena + desc.Offset;

	if (desc.Inline)
	{
		std::byte* payload = entry + SlotHeaderBytes;
		ReleaseInlinePayload(desc.Shape, payload, gc);

		if (value.IsInline)
		{
			std::memcpy(payload, value.Data, desc.Shape->Size);
		}
		else
		{
			ObjectInstance* box = value.AsObject();
			if (box != nullptr)
			{
				std::memcpy(payload, box->getMemory(), desc.Shape->Size);
				gc.DestroyInstance(box);
			}
			else
			{
				std::memset(payload, 0, desc.Shape->Size);
			}
		}

		AdoptInlinePayload(desc.Shape, payload);
		return;
	}

	ObjectInstance* adopted = value.IsInline ? WrapPayload(value.Shape, value.Data) : value.AsObject();
	if (adopted != nullptr)
		adopted->IncrementReference();

	ObjectInstance* old = PayloadRead(entry);
	PayloadRef(entry) = adopted;

	if (old != nullptr)
		gc.DestroyInstance(old);
}

void CallStackFrame::CopyArgumentPayloads(ObjectInstance** dst, std::size_t count)
{
	if (count > LocalSlots.size())
		count = LocalSlots.size();

	for (std::size_t i = 0; i < count; i++)
	{
		const LocalSlotDesc& desc = LocalSlots[i];
		std::byte* entry = Arena + desc.Offset;
		dst[i] = desc.Inline ? WrapPayload(desc.Shape, entry + SlotHeaderBytes) : PayloadRead(entry);
	}
}

void CallStackFrame::DrainEvalReferences(GarbageCollector& gc)
{
	while (EvalSize > 0)
	{
		StackValue value = PopValue();
		ReleaseValue(value, gc);
	}
}

void CallStackFrame::DrainLocalReferences(GarbageCollector& gc)
{
	for (const LocalSlotDesc& desc : LocalSlots)
	{
		std::byte* entry = Arena + desc.Offset;
		if (desc.Inline)
		{
			ReleaseInlinePayload(desc.Shape, entry + SlotHeaderBytes, gc);
		}
		else
		{
			ObjectInstance* value = PayloadRead(entry);
			if (value != nullptr)
				ReleaseBoxedPayload(value, gc);
		}
	}
}

void CallStackFrame::DrainReferences(GarbageCollector& gc)
{
	DrainEvalReferences(gc);
	DrainLocalReferences(gc);
}

CallStackFrame::~CallStackFrame()
{
	if (!ArenaIsTrailing && Arena != nullptr)
		mi_free(Arena);

	Arena = nullptr;
	ArenaBytes = 0;
	ReturnSlot = nullptr;
	LocalRegionEnd = nullptr;
	EvalEntries = nullptr;
	EvalCapacityBytes = 0;
	EvalCursorBytes = 0;
	EvalSize = 0;
	ArenaIsTrailing = false;
	Method = nullptr;
	PreviousFrame = nullptr;
}
