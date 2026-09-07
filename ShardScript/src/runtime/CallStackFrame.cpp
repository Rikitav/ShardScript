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
	static constexpr std::size_t Align(std::size_t value)
	{
		return (value + sizeof(void*) - 1) & ~(sizeof(void*) - 1);
	}

	static inline TypeShape* EntryShape(const std::byte* entry)
	{
		return *reinterpret_cast<TypeShape* const*>(entry);
	}

	static inline std::size_t EntryPayloadBytes(TypeShape* shape)
	{
		return shape != nullptr && !shape->IsReferenceType()
			? Align(shape->Size)
			: CallStackFrame::ReferencePayloadBytes;
	}

	static inline std::size_t EntryStride(const std::byte* entry)
	{
		return CallStackFrame::SlotHeaderBytes + EntryPayloadBytes(EntryShape(entry));
	}

	static void AdoptInlinePayload(const TypeShape* shape, std::byte* payload)
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
			{
				ObjectInstance gcInstance(fieldShape, fieldValue);
				gcInstance.IncrementReference();
			}
		}
	}

	static void ReleaseInlinePayload(const TypeShape* shape, std::byte* payload, GarbageCollector& gc)
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
				gc.DestroyInstance(ObjectInstance(fieldShape, fieldValue));
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

std::shared_ptr<CallStackFrame> CallStackFrame::Create(const VirtualMachine* host, const CallStackFrame* previousFrame, const MethodSymbol* method, const std::vector<TypeSymbol*>& typeArguments)
{
	TypeShapeCache& shapes = *host->GetProgram().TypeShapes;

	const std::size_t localsCount = method->GetEvalStackLocalsCount();
	std::vector<LocalSlotDesc> slotDescs;
	slotDescs.reserve(localsCount);

	TypeShape* returnShape = ResolveObjectShape(method->ReturnType, shapes);
	const std::size_t returnStride = SlotHeaderBytes + (returnShape->IsReferenceType() ? ReferencePayloadBytes : Align(returnShape->Size));

	std::uint32_t offset = static_cast<std::uint32_t>(returnStride);
	for (std::size_t slot = 0; slot < localsCount; slot++)
	{
		TypeSymbol* type = ResolveSlotType(*method, static_cast<std::uint16_t>(slot), typeArguments);
		TypeShape* shape = ResolveObjectShape(type, shapes);

		const bool isInline = (shape != nullptr && type != nullptr && !type->IsReferenceType());
		const std::size_t stride = SlotHeaderBytes + (isInline ? Align(shape->Size) : ReferencePayloadBytes);

		slotDescs.push_back(LocalSlotDesc{ shape, offset });
		offset += static_cast<std::uint32_t>(stride);
	}

	const std::size_t localsBytes = offset - returnStride;

	const std::size_t evalMaxPayload = std::max<std::size_t>(ReferencePayloadBytes, method->Layout.EvalSlotPayload);
	const std::size_t evalEntryStride = SlotHeaderBytes + Align(evalMaxPayload);
	const std::size_t evalEntries = method->Layout.IsComplete ? std::max<std::size_t>(method->Layout.MaxEvalDepth, 8) : 64;
	const std::size_t evalCapacityBytes = evalEntries * evalEntryStride;
	const std::size_t arenaBytes = returnStride + localsBytes + evalCapacityBytes;

	void* block = mi_zalloc(sizeof(CallStackFrame) + arenaBytes);
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

ObjectInstance CallStackFrame::PushInlineUninitialized(const TypeShape* shape)
{
	if (shape == nullptr)
		throw std::runtime_error("Cannot push an inline value without a type shape");

	std::byte* entry = EvalEntries + EvalCursorBytes;
	std::byte* payload = entry + SlotHeaderBytes;

	EvalOffsets.push_back(static_cast<std::uint32_t>(EvalCursorBytes));
	EvalCursorBytes += SlotHeaderBytes + Align(shape->Size);

	std::memcpy(entry, &shape, sizeof(shape));
	return ObjectInstance(shape, payload);
}

ObjectInstance CallStackFrame::PushInline(const TypeShape* shape, const void* payloadBytes)
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
	std::byte* payload = entry + SlotHeaderBytes;

	EvalOffsets.push_back(EvalCursorBytes);
	EvalCursorBytes += BoxedEntryStride;

	const TypeShape* header = value.getShape();
	std::byte* stored = value.getMemory();

	std::memcpy(entry, &header, sizeof(header));
	std::memcpy(entry + SlotHeaderBytes, &stored, sizeof(stored));
	return value;
}

ObjectInstance CallStackFrame::PushStack(ObjectInstance value)
{
	const TypeShape* info = value.getShape();
	if (info != nullptr && !info->IsReferenceType())
		return PushInline(value.getShape(), value.getMemory());

	return PushReference(value);
}

ObjectInstance CallStackFrame::PopStack()
{
	ObjectInstance result = PeekStack();
	EvalOffsets.pop_back();
	return result;
}

ObjectInstance CallStackFrame::PeekStack()
{
	const std::uint32_t offset = EvalOffsets.back();
	std::byte* entry = EvalEntries + offset;
	std::byte* payload = entry + SlotHeaderBytes;

	TypeShape* shape = EntryShape(entry);
	if (shape == nullptr)
		throw undefined_behaviour("EvalStack: TypeShape was null");

	if (shape->IsReferenceType())
		return ObjectInstance(shape, *reinterpret_cast<std::byte**>(payload));

	return ObjectInstance(shape, payload);
}

ObjectInstance CallStackFrame::GetLocal(std::uint16_t slot)
{
	LocalSlotDesc& desc = LocalSlots.at(slot);

	std::byte* entry = Arena + desc.Offset;
	std::byte* payload = entry + SlotHeaderBytes;

	TypeShape* shape = EntryShape(entry);
	if (shape == nullptr)
		throw undefined_behaviour("EvalStack: TypeShape was null");

	if (shape->IsReferenceType())
		return ObjectInstance(shape, *reinterpret_cast<std::byte**>(payload));

	return ObjectInstance(shape, payload);
}

void CallStackFrame::SetLocal(std::uint16_t slot, const ObjectInstance& value)
{
	LocalSlotDesc& desc = LocalSlots.at(slot);
	desc.Shape = value.getShape();

	std::byte* entry = Arena + desc.Offset;
	std::byte* payload = entry + SlotHeaderBytes;

	if (value.getShape()->IsReferenceType())
	{
		std::byte* old = *reinterpret_cast<std::byte**>(payload);
		if (old != nullptr)
			Host->GetGarbageCollector().DestroyInstance(ObjectInstance(desc.Shape, old));

		std::byte* stored = value.getMemory();
		if (stored != nullptr)
			value.IncrementReference();

		std::memcpy(entry, &desc.Shape, sizeof(TypeShape*));
		std::memcpy(payload, &stored, sizeof(std::byte*));
	}
	else
	{
		std::memcpy(entry, &desc.Shape, sizeof(TypeShape*));
		ReleaseInlinePayload(desc.Shape, payload, Host->GetGarbageCollector());
		AdoptInlinePayload(desc.Shape, payload);
	}
}

void CallStackFrame::CopyArgumentPayloads()
{
	CallStackFrame* callingFrame = const_cast<CallStackFrame*>(PreviousFrame);
	std::size_t argsCount = Method->GetEvalStackArgumentsCount();

	for (std::size_t i = 0; i < argsCount; i++)
	{
		ObjectInstance argument = callingFrame->PopStack();
		LocalSlotDesc& desc = LocalSlots.at(i);

		std::byte* entry = Arena + desc.Offset;
		std::byte* payload = entry + SlotHeaderBytes;

		const TypeShape* shape = argument.getShape();
		desc.Shape = shape;

		std::memcpy(entry, &shape, SlotHeaderBytes);

		if (shape->IsReferenceType())
		{
			std::byte* data = argument.getMemory();
			std::memcpy(payload, &data, ReferencePayloadBytes);
		}
		else
		{
			AdoptInlinePayload(desc.Shape, payload);
		}
	}
}

ObjectInstance CallStackFrame::ReturnView() const
{
	if (Method->ReturnType == SymbolTable::Primitives::Void)
		throw undefined_behaviour("PlaceReturned: method returns void");

	std::byte* entry = ReturnSlotMemory();
	std::byte* payload = entry + SlotHeaderBytes;

	const TypeShape* shape = ReturnShape();
	if (shape->IsReferenceType())
	{
		return ObjectInstance(shape, *reinterpret_cast<std::byte**>(payload));
	}
	else
	{
		return ObjectInstance(shape, payload);
	}
}

void CallStackFrame::PlaceReturned(ObjectInstance value)
{
	if (Method->ReturnType == SymbolTable::Primitives::Void)
		throw undefined_behaviour("PlaceReturned: method returns void");

	if (InterruptionReason == FrameInterruptionReason::ValueReturned)
		throw undefined_behaviour("PlaceReturned: return value already placed");

	const TypeShape* returnShape = ReturnShape();
	const TypeShape* valueShape = value.getShape();

	if (returnShape != valueShape)
		throw undefined_behaviour("PlaceReturned: shapes mismatched");

	std::byte* stored = value.getMemory();
	std::byte* entry = ReturnSlotMemory();
	std::byte* payload = entry + CallStackFrame::SlotHeaderBytes;

	if (returnShape->IsReferenceType())
	{
		value.IncrementReference();
		std::memcpy(entry, &returnShape, sizeof(TypeShape*));
		std::memcpy(payload, &stored, sizeof(std::byte*));
	}
	else
	{
		std::memcpy(entry, &returnShape, sizeof(TypeShape*));
		std::memcpy(payload, stored, returnShape->Size);
	}

	InterruptionReason = FrameInterruptionReason::ValueReturned;
}

void CallStackFrame::DrainEvalReferences(GarbageCollector& gc)
{
	while (EvalOffsets.size() > 0)
	{
		ObjectInstance value = PopStack();
		ReleaseValue(value, gc);
	}
}

void CallStackFrame::DrainLocalReferences(GarbageCollector& gc)
{
	for (const LocalSlotDesc& desc : LocalSlots)
	{
		std::byte* entry = Arena + desc.Offset;

		if (!desc.Shape->IsReferenceType())
		{
			ReleaseInlinePayload(desc.Shape, entry + SlotHeaderBytes, gc);
		}
		else
		{
			std::byte* stored = nullptr;
			std::memcpy(&stored, entry + SlotHeaderBytes, sizeof(stored));
			if (stored != nullptr)
			{
				ObjectInstance gcInstance(desc.Shape, stored);
				gc.DestroyInstance(gcInstance);
			}
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
