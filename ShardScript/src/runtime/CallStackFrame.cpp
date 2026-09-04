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

	// One extra pointer slot reserved at the base for the method's return value;
	const std::size_t arenaBytes = (1 + localsCount + evalCapacity) * sizeof(ObjectInstance*);

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
	ArenaBytes = (1 + LocalCapacity + EvalCapacity) * sizeof(ObjectInstance*);
	ArenaIsTrailing = true;

	ReturnSlotStorage = reinterpret_cast<ObjectInstance**>(Arena);
	ReturnSlotStorage[0] = nullptr;

	LocalSlots = ReturnSlotStorage + 1;
	std::fill(LocalSlots, LocalSlots + LocalCapacity, nullptr);

	EvalSlots = LocalSlots + LocalCapacity;
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
		std::size_t sideBytes = (1 + newLocalCapacity + newEvalCapacity) * sizeof(ObjectInstance*);
		std::byte* sideArena = static_cast<std::byte*>(mi_malloc(sideBytes));
		if (sideArena == nullptr)
			throw std::runtime_error("Failed to grow call stack frame arena");

		ObjectInstance** newReturnSlot = reinterpret_cast<ObjectInstance**>(sideArena);
		ObjectInstance** newLocals = newReturnSlot + 1;
		ObjectInstance** newEval = newLocals + newLocalCapacity;

		newReturnSlot[0] = ReturnSlotStorage[0];
		std::fill(newLocals, newLocals + newLocalCapacity, nullptr);
		std::memcpy(newLocals, LocalSlots, LocalCapacity * sizeof(ObjectInstance*));
		std::memcpy(newEval, EvalSlots, EvalSize * sizeof(ObjectInstance*));

		Arena = sideArena;
		ArenaBytes = sideBytes;
		ArenaIsTrailing = false;
		ReturnSlotStorage = newReturnSlot;
		LocalSlots = newLocals;
		LocalCapacity = newLocalCapacity;
		EvalSlots = newEval;
		EvalCapacity = newEvalCapacity;
		return;
	}

	const std::size_t oldEvalSize = EvalSize;
	std::size_t newBytes = (1 + newLocalCapacity + newEvalCapacity) * sizeof(ObjectInstance*);
	std::byte* newArena = static_cast<std::byte*>(mi_realloc(Arena, newBytes));
	if (newArena == nullptr)
		throw std::runtime_error("Failed to grow call stack frame arena");

	ObjectInstance** newReturnSlot = reinterpret_cast<ObjectInstance**>(newArena);
	ObjectInstance** newLocals = newReturnSlot + 1;
	ObjectInstance** newEval = newLocals + newLocalCapacity;

	// The old interior pointers must not be touched after realloc — the old block may have moved.
	std::fill(newLocals + LocalCapacity, newLocals + newLocalCapacity, nullptr);
	ObjectInstance** oldEval = newLocals + LocalCapacity;
	std::memmove(newEval, oldEval, oldEvalSize * sizeof(ObjectInstance*));

	Arena = newArena;
	ArenaBytes = newBytes;
	ReturnSlotStorage = newReturnSlot;
	LocalSlots = newLocals;
	LocalCapacity = newLocalCapacity;
	EvalSlots = newEval;
	EvalCapacity = newEvalCapacity;
}

void CallStackFrame::PushStack(ObjectInstance* value)
{
	if (EvalSize >= EvalCapacity)
		GrowArena(LocalCapacity, EvalCapacity != 0 ? EvalCapacity * 2 : 8);

	EvalSlots[EvalSize++] = value;
}

ObjectInstance* CallStackFrame::PopStack()
{
	if (EvalSize == 0)
		throw std::runtime_error("Evaluation stack underflow");

	return EvalSlots[--EvalSize];
}

ObjectInstance* CallStackFrame::PeekStack()
{
	if (EvalSize == 0)
		throw std::runtime_error("Evaluation stack underflow");

	return EvalSlots[EvalSize - 1];
}

ObjectInstance* CallStackFrame::GetLocal(std::uint16_t slot) const
{
	if (static_cast<std::size_t>(slot) >= LocalCapacity)
		return nullptr;

	return LocalSlots[slot];
}

ObjectInstance*& CallStackFrame::LocalRef(std::uint16_t slot)
{
	if (static_cast<std::size_t>(slot) >= LocalCapacity)
		GrowArena(static_cast<std::size_t>(slot) + 1, EvalCapacity);

	return LocalSlots[slot];
}

CallStackFrame::~CallStackFrame()
{
	if (!ArenaIsTrailing && Arena != nullptr)
		mi_free(Arena);

	Arena = nullptr;
	ReturnSlotStorage = nullptr;
	LocalSlots = nullptr;
	EvalSlots = nullptr;
	LocalCapacity = 0;
	EvalCapacity = 0;
	EvalSize = 0;
	ArenaBytes = 0;
	ArenaIsTrailing = false;
	Method = nullptr;
	PreviousFrame = nullptr;
}
