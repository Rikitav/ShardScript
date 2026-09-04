#include <shard/runtime/CallStackFrame.hpp>
#include <shard/runtime/ObjectInstance.hpp>

#include <shard/semantic/symbols/TypeParameterSymbol.hpp>

#include <mimalloc.h>

#include <algorithm>
#include <cstring>
#include <new>
#include <stdexcept>

using namespace shard;

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

std::shared_ptr<CallStackFrame> CallStackFrame::Create(const VirtualMachine* host, CallStackFrame* previousFrame, MethodSymbol* method)
{
	const std::size_t localsCount = method->GetEvalStackLocalsCount();
	const std::size_t evalCapacity = method->Layout.IsComplete
		? std::max<std::size_t>(method->Layout.MaxEvalDepth, 8) : 64;

	// One extra entry reserved at the base for the method's return value;
	const std::size_t arenaBytes = (1 + localsCount + evalCapacity) * CallStackFrame::SlotStride;

	void* block = mi_malloc(sizeof(CallStackFrame) + arenaBytes);
	if (block == nullptr)
		throw std::runtime_error("Failed to allocate call stack frame");

	CallStackFrame* frame = new (block) CallStackFrame(host, previousFrame, method);

	std::shared_ptr<CallStackFrame> result(frame, [](CallStackFrame* ptr)
	{
		ptr->~CallStackFrame();
		mi_free(ptr);
	});

	frame->InitializeArena(localsCount, evalCapacity);
	return result;
}

void CallStackFrame::InitializeArena(std::size_t localsCount, std::size_t evalCapacity)
{
	if (Arena != nullptr)
		return; // already initialized (e.g. frame reused by a host)

	LocalCapacity = localsCount;
	EvalCapacity = evalCapacity;
	EvalSize = 0;

	// The arena trails the frame object inside the single Create() allocation.
	Arena = reinterpret_cast<std::byte*>(this) + sizeof(CallStackFrame);
	ArenaBytes = (1 + LocalCapacity + EvalCapacity) * SlotStride;
	ArenaIsTrailing = true;

	ReturnSlot = Arena;
	std::memset(ReturnSlot, 0, SlotStride);

	LocalEntries = ReturnSlot + SlotStride;
	std::memset(LocalEntries, 0, LocalCapacity * SlotStride);

	EvalEntries = LocalEntries + LocalCapacity * SlotStride;
}

void CallStackFrame::GrowArena(std::size_t newLocalCapacity, std::size_t newEvalCapacity)
{
	if (newLocalCapacity <= LocalCapacity && newEvalCapacity <= EvalCapacity)
		return;

	if (newLocalCapacity < LocalCapacity)
		newLocalCapacity = LocalCapacity;

	if (newEvalCapacity < EvalCapacity)
		newEvalCapacity = EvalCapacity;

	if (ArenaIsTrailing)
	{
		// Migrate the arena to a side allocation instead; the frame stays put.
		std::size_t sideBytes = (1 + newLocalCapacity + newEvalCapacity) * SlotStride;
		std::byte* sideArena = static_cast<std::byte*>(mi_malloc(sideBytes));
		if (sideArena == nullptr)
			throw std::runtime_error("Failed to grow call stack frame arena");

		std::byte* newReturnSlot = sideArena;
		std::byte* newLocals = newReturnSlot + SlotStride;
		std::byte* newEval = newLocals + newLocalCapacity * SlotStride;

		std::memcpy(newReturnSlot, ReturnSlot, SlotStride);
		std::memset(newLocals, 0, newLocalCapacity * SlotStride);
		std::memcpy(newLocals, LocalEntries, LocalCapacity * SlotStride);
		std::memcpy(newEval, EvalEntries, EvalSize * SlotStride);

		Arena = sideArena;
		ArenaBytes = sideBytes;
		ArenaIsTrailing = false;
		ReturnSlot = newReturnSlot;
		LocalEntries = newLocals;
		LocalCapacity = newLocalCapacity;
		EvalEntries = newEval;
		EvalCapacity = newEvalCapacity;
		return;
	}

	const std::size_t oldEvalSize = EvalSize;
	std::size_t newBytes = (1 + newLocalCapacity + newEvalCapacity) * SlotStride;
	std::byte* newArena = static_cast<std::byte*>(mi_realloc(Arena, newBytes));
	if (newArena == nullptr)
		throw std::runtime_error("Failed to grow call stack frame arena");

	std::byte* newReturnSlot = newArena;
	std::byte* newLocals = newReturnSlot + SlotStride;
	std::byte* newEval = newLocals + newLocalCapacity * SlotStride;

	// The old interior pointers must not be touched after realloc — the old block may have moved.
	std::memset(newLocals + LocalCapacity * SlotStride, 0, (newLocalCapacity - LocalCapacity) * SlotStride);
	std::byte* oldEval = newLocals + LocalCapacity * SlotStride;
	std::memmove(newEval, oldEval, oldEvalSize * SlotStride);

	Arena = newArena;
	ArenaBytes = newBytes;
	ReturnSlot = newReturnSlot;
	LocalEntries = newLocals;
	LocalCapacity = newLocalCapacity;
	EvalEntries = newEval;
	EvalCapacity = newEvalCapacity;
}

void CallStackFrame::PushStack(ObjectInstance* value)
{
	if (EvalSize >= EvalCapacity)
		GrowArena(LocalCapacity, EvalCapacity != 0 ? EvalCapacity * 2 : 8);

	std::byte* entry = EvalEntries + EvalSize * SlotStride;
	*reinterpret_cast<TypeShape**>(entry) = value != nullptr ? value->getShape() : nullptr;
	PayloadRef(entry) = value;
	EvalSize++;
}

ObjectInstance* CallStackFrame::PopStack()
{
	if (EvalSize == 0)
		throw std::runtime_error("Evaluation stack underflow");

	EvalSize--;
	return PayloadRead(EvalEntries + EvalSize * SlotStride);
}

ObjectInstance* CallStackFrame::PeekStack()
{
	if (EvalSize == 0)
		throw std::runtime_error("Evaluation stack underflow");

	return PayloadRead(EvalEntries + (EvalSize - 1) * SlotStride);
}

ObjectInstance* CallStackFrame::GetLocal(std::uint16_t slot) const
{
	if (static_cast<std::size_t>(slot) >= LocalCapacity)
		return nullptr;

	return PayloadRead(LocalEntries + static_cast<std::size_t>(slot) * SlotStride);
}

ObjectInstance*& CallStackFrame::LocalRef(std::uint16_t slot)
{
	if (static_cast<std::size_t>(slot) >= LocalCapacity)
		GrowArena(static_cast<std::size_t>(slot) + 1, EvalCapacity);

	return PayloadRef(LocalEntries + static_cast<std::size_t>(slot) * SlotStride);
}

void CallStackFrame::CopyArgumentPayloads(ObjectInstance** dst, std::size_t count) const
{
	if (count > LocalCapacity)
		count = LocalCapacity;

	for (std::size_t i = 0; i < count; i++)
		dst[i] = PayloadRead(LocalEntries + i * SlotStride);
}

CallStackFrame::~CallStackFrame()
{
	if (!ArenaIsTrailing && Arena != nullptr)
		mi_free(Arena);

	Arena = nullptr;
	ReturnSlot = nullptr;
	LocalEntries = nullptr;
	EvalEntries = nullptr;
	LocalCapacity = 0;
	EvalCapacity = 0;
	EvalSize = 0;
	ArenaBytes = 0;
	ArenaIsTrailing = false;
	Method = nullptr;
	PreviousFrame = nullptr;
}
