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

			std::byte* fieldValue = nullptr;
			std::memcpy(&fieldValue, payload + shape->GetOffset(slot), sizeof(fieldValue));
			if (fieldValue != nullptr)
				ObjectInstance(fieldShape->BaseType, fieldShape, fieldValue).IncrementReference();
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

			std::byte* fieldValue = nullptr;
			std::memcpy(&fieldValue, payload + shape->GetOffset(slot), sizeof(fieldValue));
			if (fieldValue != nullptr)
				gc.DestroyInstance(ObjectInstance(fieldShape->BaseType, fieldShape, fieldValue));
		}
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

	// Resolves the shape of the object the slot holds (both value and reference
	// types); returns nullptr only when the type is unknown.
	static TypeShape* ResolveObjectShape(TypeSymbol* type, TypeShapeCache& shapes)
	{
		if (type == nullptr)
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

		if (baseType == nullptr)
			return nullptr;

		return shapes.GetOrCreateShape(baseType, genericArgs);
	}

	// Shape for an inline (by-value) slot; nullptr for references/void/unknown.
	static TypeShape* ResolveInlineShape(TypeSymbol* type, TypeShapeCache& shapes)
	{
		if (type == nullptr || type->IsReferenceType())
			return nullptr;

		return ResolveObjectShape(type, shapes);
	}
}

CallStackFrame::~CallStackFrame() = default;

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

	TypeShape* returnShape = ResolveInlineShape(method->ReturnType, shapes);
	const std::size_t returnStride = SlotHeaderBytes + (returnShape != nullptr ? Align(returnShape->Size) : ReferencePayloadBytes);

	std::uint32_t offset = static_cast<std::uint32_t>(returnStride);
	for (std::size_t slot = 0; slot < localsCount; slot++)
	{
		TypeSymbol* type = ResolveSlotType(*method, static_cast<std::uint16_t>(slot), typeArguments);
		TypeShape* shape = ResolveObjectShape(type, shapes);
		const bool isInline = (shape != nullptr && type != nullptr && !type->IsReferenceType());
		const std::size_t stride = SlotHeaderBytes + (isInline ? Align(shape->Size) : ReferencePayloadBytes);

		slotDescs.push_back(LocalSlotDesc{ shape, offset, isInline });
		offset += static_cast<std::uint32_t>(stride);
	}

	const std::size_t localsBytes = offset - returnStride;

	const std::size_t evalMaxPayload = std::max<std::size_t>(ReferencePayloadBytes, method->Layout.EvalSlotPayload);
	const std::size_t evalEntryStride = SlotHeaderBytes + Align(evalMaxPayload);
	const std::size_t evalEntries = method->Layout.IsComplete
		? std::max<std::size_t>(method->Layout.MaxEvalDepth, 8) : 64;

	const std::size_t evalCapacityBytes = evalEntries * evalEntryStride;
	const std::size_t arenaBytes = returnStride + localsBytes + evalCapacityBytes;

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

	frame->ReturnSlot = frame->Arena;
	frame->ReturnSlotShape = returnShape;
	frame->LocalSlots = std::move(slotDescs);
	frame->LocalRegionEnd = frame->Arena + returnStride + localsBytes;
	frame->EvalEntries = frame->LocalRegionEnd;
	frame->EvalCapacityBytes = evalCapacityBytes;
	frame->EvalOffsets.reserve(evalEntries);

	return result;
}

ObjectInstance CallStackFrame::PushInlineUninitialized(TypeShape* shape)
{
	if (shape == nullptr)
		throw std::runtime_error("Cannot push an inline value without a type shape");

	const std::size_t stride = SlotHeaderBytes + Align(shape->Size);

	std::byte* entry = EvalEntries + EvalCursorBytes;
	EvalOffsets.push_back(static_cast<std::uint32_t>(EvalCursorBytes));
	EvalCursorBytes += stride;
	EvalSize++;

	*reinterpret_cast<TypeShape**>(entry) = shape;

	return ObjectInstance(shape->BaseType, shape, entry + SlotHeaderBytes);
}

ObjectInstance CallStackFrame::PushInline(TypeShape* shape, const void* payloadBytes)
{
	ObjectInstance payload = PushInlineUninitialized(shape);
	if (payloadBytes != nullptr)
		std::memcpy(payload.getMemory(), payloadBytes, shape->Size);

	AdoptInlinePayload(shape, payload.getMemory());
	return payload;
}

ObjectInstance CallStackFrame::PushReference(ObjectInstance value)
{
	std::byte* entry = EvalEntries + EvalCursorBytes;
	EvalOffsets.push_back(static_cast<std::uint32_t>(EvalCursorBytes));
	EvalCursorBytes += BoxedEntryStride;
	EvalSize++;

	*reinterpret_cast<TypeShape**>(entry) = value.getShape();

	std::byte* stored = value.getMemory();
	std::memcpy(entry + SlotHeaderBytes, &stored, sizeof(stored));

	return value;
}

ObjectInstance CallStackFrame::PushCopy(const ObjectInstance& value)
{
	const TypeSymbol* info = value.getInfo();
	if (info != nullptr && !info->IsReferenceType())
		return PushInline(value.getShape(), value.getMemory());

	return PushReference(value);
}

ObjectInstance CallStackFrame::PushStack(ObjectInstance value)
{
	return PushReference(value);
}

ObjectInstance CallStackFrame::PopValue()
{
	const std::uint32_t offset = EvalOffsets.back();
	EvalOffsets.pop_back();
	EvalSize--;

	std::byte* entry = EvalEntries + offset;
	EvalCursorBytes = offset;

	TypeShape* shape = EntryShape(entry);
	if (shape == nullptr || shape->IsReferenceType())
	{
		std::byte* stored = nullptr;
		std::memcpy(&stored, entry + SlotHeaderBytes, sizeof(stored));
		return shape != nullptr ? ObjectInstance(shape->BaseType, shape, stored) : ObjectInstance();
	}

	return ObjectInstance(shape->BaseType, shape, entry + SlotHeaderBytes);
}

ObjectInstance CallStackFrame::TopValue()
{
	const std::uint32_t offset = EvalOffsets.back();
	std::byte* entry = EvalEntries + offset;

	TypeShape* shape = EntryShape(entry);
	if (shape == nullptr || shape->IsReferenceType())
	{
		std::byte* stored = nullptr;
		std::memcpy(&stored, entry + SlotHeaderBytes, sizeof(stored));
		return shape != nullptr ? ObjectInstance(shape->BaseType, shape, stored) : ObjectInstance();
	}

	return ObjectInstance(shape->BaseType, shape, entry + SlotHeaderBytes);
}

ObjectInstance CallStackFrame::PopStack()
{
	ObjectInstance value = PopValue();
	if (value.getInfo() != nullptr && !value.getInfo()->IsReferenceType())
		throw std::runtime_error("Popped an inline eval entry as a reference");

	return value;
}

ObjectInstance CallStackFrame::PeekStack()
{
	ObjectInstance value = TopValue();
	if (value.getInfo() != nullptr && !value.getInfo()->IsReferenceType())
		throw std::runtime_error("Peeked an inline eval entry as a reference");

	return value;
}

ObjectInstance CallStackFrame::GetLocal(std::uint16_t slot)
{
	const LocalSlotDesc& desc = LocalSlots.at(slot);
	std::byte* entry = Arena + desc.Offset;

	if (desc.Inline)
		return ObjectInstance(desc.Shape->BaseType, desc.Shape, entry + SlotHeaderBytes);

	std::byte* stored = nullptr;
	std::memcpy(&stored, entry + SlotHeaderBytes, sizeof(stored));
	if (desc.Shape == nullptr)
		return ObjectInstance(nullptr, nullptr, stored);

	return ObjectInstance(desc.Shape->BaseType, desc.Shape, stored);
}

void CallStackFrame::SetLocal(std::uint16_t slot, const ObjectInstance& value, GarbageCollector& gc)
{
	const LocalSlotDesc& desc = LocalSlots.at(slot);
	std::byte* entry = Arena + desc.Offset;
	std::byte* payload = entry + SlotHeaderBytes;

	if (desc.Inline)
	{
		*reinterpret_cast<TypeShape**>(entry) = desc.Shape;

		ReleaseInlinePayload(desc.Shape, payload, gc);
		std::memcpy(payload, value.getMemory(), desc.Shape->Size);
		AdoptInlinePayload(desc.Shape, payload);
		return;
	}

	std::byte* old = nullptr;
	std::memcpy(&old, payload, sizeof(old));
	if (old != nullptr)
		gc.DestroyInstance(ObjectInstance(desc.Shape != nullptr ? desc.Shape->BaseType : nullptr, desc.Shape, old));

	std::byte* stored = value.getMemory();
	if (stored != nullptr)
		value.IncrementReference();

	*reinterpret_cast<TypeShape**>(entry) = desc.Shape;
	std::memcpy(payload, &stored, sizeof(stored));
}

void CallStackFrame::CopyArgumentPayloads(ObjectInstance* dst, std::size_t count)
{
	if (count > LocalSlots.size())
		count = LocalSlots.size();

	for (std::size_t i = 0; i < count; i++)
	{
		const LocalSlotDesc& desc = LocalSlots[i];
		std::byte* entry = Arena + desc.Offset;

		if (desc.Inline)
		{
			dst[i] = ObjectInstance(desc.Shape->BaseType, desc.Shape, entry + SlotHeaderBytes);
		}
		else
		{
			std::byte* stored = nullptr;
			std::memcpy(&stored, entry + SlotHeaderBytes, sizeof(stored));
			dst[i] = desc.Shape != nullptr
				? ObjectInstance(desc.Shape->BaseType, desc.Shape, stored)
				: ObjectInstance(nullptr, nullptr, stored);
		}
	}
}

void CallStackFrame::DrainEvalReferences(GarbageCollector& gc)
{
	while (EvalSize > 0)
	{
		ObjectInstance value = PopValue();
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
			std::byte* stored = nullptr;
			std::memcpy(&stored, entry + SlotHeaderBytes, sizeof(stored));
			if (stored != nullptr)
				gc.DestroyInstance(ObjectInstance(desc.Shape != nullptr ? desc.Shape->BaseType : nullptr, desc.Shape, stored));
		}
	}
}

void CallStackFrame::DrainReferences(GarbageCollector& gc)
{
	DrainEvalReferences(gc);
	DrainLocalReferences(gc);
}

void CallStackFrame::ReleaseValue(const ObjectInstance& value, GarbageCollector& gc)
{
	const TypeSymbol* info = value.getInfo();
	if (info != nullptr && !info->IsReferenceType())
		ReleaseInlinePayload(value.getShape(), value.getMemory(), gc);
	else
		gc.DestroyInstance(value);
}

void CallStackFrame::DiscardValue(const ObjectInstance& value, GarbageCollector& gc)
{
	const TypeSymbol* info = value.getInfo();
	if (info != nullptr && !info->IsReferenceType())
		ReleaseInlinePayload(value.getShape(), value.getMemory(), gc);
	else
		gc.CollectInstance(value);
}
