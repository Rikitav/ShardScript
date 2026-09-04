#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/runtime/ObjectInstance.hpp>

#include <shard/semantic/symbols/TypeSymbol.hpp>
#include <shard/semantic/symbols/MethodSymbol.hpp>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <vector>

// ============================================================================
// CallStackFrame — one invocation of a method on the virtual machine's call
// stack. Holds the method's state for the duration of the call: argument and
// local-variable slots, the evaluation stack the bytecode pushes to and pops
// from, exception-handler and defer bookkeeping, and the resolved type
// arguments for generic methods.
//
// ----------------------------------------------------------------------------
// MEMORY LAYOUT
//
// A frame is ONE allocation. `CallStackFrame::Create` mi_mallocs
//
//     sizeof(CallStackFrame) + arenaBytes
//
// placement-news the object at the start of the block, and hands it to a
// shared_ptr whose deleter runs the destructor and mi_frees the block. The
// arena trails the object:
//
//     [CallStackFrame object][return slot][locals region][eval region]
//     ^-- this               ^-- Arena = this + sizeof(CallStackFrame)
//
// `sizeof` covers the complete object including tail padding, so the arena
// base does not depend on member declaration order. The frame is freed when
// the shared_ptr dies (normally at PopFrame; async frames with pending tasks
// survive via PendingTaskCount until the tasks release them).
//
// Every slot in the three regions is a TAGGED, HEADER-DRIVEN byte entry:
//
//     [TypeShape* shape][payload ...]
//
// The header points at the slot value's concrete TypeShape (the
// per-instantiation layout metadata — see TypeShape/TypeShapeCache), so any
// slot can be interpreted without touching the value itself: field offsets,
// size, generic arguments. The payload layout is decided by the shape's
// storage kind:
//
//   reference-kind   payload = ObjectInstance* (one heap pointer)
//   value-kind       payload = byte[shape->Size] stored inline
//                    (Stage 4 items 3/4 — nothing produces inline payloads
//                    yet; every slot today is reference-kind)
//
// With all slots reference-kind the entry stride is uniform:
//
//     SlotStride = sizeof(TypeShape*) + sizeof(ObjectInstance*)
//
// A zeroed entry ({nullptr, nullptr}) reads as a null value, which keeps the
// pre-zeroed regions meaningful. Uniform stride also means frame creation
// does NOT need the method's type arguments yet — every slot costs the same.
// Once value-kind payloads land, the stride becomes per-slot
// (sizeof(TypeShape*) + max(sizeof(ObjectInstance*), shape->Size)) and
// Create() must receive the resolved type arguments BEFORE allocation,
// because generic instantiations change slot sizes.
//
//   Return slot   — one reserved entry at the arena base, sized for the
//                   method's return value. Unused until Stage 5 wires
//                   foreign-function value returns through it.
//   Locals region — argument slots followed by local-variable slots,
//                   pre-zeroed. Argument transfer on invocation writes
//                   directly into the front of this region; external
//                   methods receive their arguments as an ArgumentsSpan of
//                   ObjectInstance* — the VM copies the argument payloads
//                   into a scratch buffer per external call, so dependent
//                   libraries keep the contiguous span ABI.
//   Eval region   — the stack bytecode operands live on. PushStack/PopStack
//                   move an EvalSize cursor (in entries) over it; values
//                   above the locals live and die here.
//
// GROWTH NEVER MOVES THE FRAME OBJECT. Raw CallStackFrame* pointers are all
// over the runtime — PreviousFrame chains, async task bindings, CurrentFrame
// results — so realloc'ing the single block in place is forbidden. If a
// region outgrows its initial capacity, GrowArena migrates the arena to a
// separate side block (copying the regions, flipping ArenaIsTrailing) and
// the frame object stays where it was. The destructor frees the arena only
// when it is such a side allocation; a trailing arena is freed together
// with the object by the Create() deleter.
//
// ----------------------------------------------------------------------------
// WHERE THE CAPACITY COMES FROM
//
// The arena is sized exactly at frame creation from metadata computed
// during bytecode emission (see MethodSymbol::FrameLayout and the
// EvalLayoutTracker instrumentation in AbstractEmiter / AsyncEmissionPass).
// Capacities are ENTRY counts; the byte size is entries * SlotStride:
//
//   locals capacity = MethodSymbol::GetEvalStackLocalsCount()
//       Argument and variable slots are assigned once, during semantic
//       analysis and emission, so the count is exact and never grows in
//       practice (LocalRef still grows defensively, matching the old
//       vector resize-on-store behavior).
//
//   eval capacity   = max(FrameLayout::MaxEvalDepth, 8)   when Layout.IsComplete
//                   = 64                                    otherwise
//       MaxEvalDepth is the peak number of eval entries the emitted
//       bytecode can hold, tracked instruction-by-instruction at emission
//       time. The floor of 8 guarantees exception dispatch always has room
//       to push the thrown exception. When the layout is unknown (tracker
//       desync / poisoned, or an external stub with no bytecode) the frame
//       starts with a default region and grows on demand — that is the only
//       path that can trigger a side migration.
//
// ============================================================================

namespace shard
{
	class VirtualMachine;

	enum class FrameInterruptionReason
	{
		None,
		ValueReturned,
		ExceptionRaised,
	};

	class SHARD_API CallStackFrame : public std::enable_shared_from_this<CallStackFrame>
	{
	public:
		struct ExceptionHandlerFrame
		{
			std::size_t HandlerOffset;
			std::size_t DeferStackBase;
		};

		const VirtualMachine* Host;
		CallStackFrame* PreviousFrame;
		MethodSymbol* Method;

		std::byte* Arena = nullptr;
		std::size_t ArenaBytes = 0;

		std::vector<TypeSymbol*> TypeArguments;
		std::vector<ExceptionHandlerFrame> ExceptionHandlers;
		std::size_t PendingTaskCount = 0;

		FrameInterruptionReason InterruptionReason = FrameInterruptionReason::None;
		ObjectInstance* InterruptionRegister = nullptr;
		ObjectInstance* CurrentException = nullptr;

		std::vector<std::size_t> DeferStack;
		std::size_t DeferDrainDepth = 0;

	private:
		CallStackFrame(const VirtualMachine* host, CallStackFrame* previousFrame, MethodSymbol* method)
			: Host(host), Method(method), PreviousFrame(previousFrame) { }

	public:
		inline bool interrupted() const
		{
			return InterruptionReason != FrameInterruptionReason::None;
		}

		TypeSymbol* ResolveType(TypeSymbol* type);

		static std::shared_ptr<CallStackFrame> Create(const VirtualMachine* host, CallStackFrame* previousFrame, MethodSymbol* method);

		static constexpr std::size_t SlotHeaderBytes = sizeof(TypeShape*);
		static constexpr std::size_t SlotPayloadBytes = sizeof(ObjectInstance*);
		static constexpr std::size_t SlotStride = SlotHeaderBytes + SlotPayloadBytes;

		void PushStack(ObjectInstance* value);
		ObjectInstance* PopStack();
		ObjectInstance* PeekStack();

		ObjectInstance* GetLocal(std::uint16_t slot) const;
		ObjectInstance*& LocalRef(std::uint16_t slot);

		inline std::size_t EvalCount() const { return EvalSize; }
		inline std::size_t LocalCount() const { return LocalCapacity; }

		void CopyArgumentPayloads(ObjectInstance** dst, std::size_t count) const;

		~CallStackFrame();

	private:
		std::byte* ReturnSlot = nullptr;
		std::byte* LocalEntries = nullptr;
		std::size_t LocalCapacity = 0;
		std::byte* EvalEntries = nullptr;
		std::size_t EvalCapacity = 0;
		std::size_t EvalSize = 0;
		bool ArenaIsTrailing = false;

		static inline ObjectInstance*& PayloadRef(std::byte* entry)
		{
			return *reinterpret_cast<ObjectInstance**>(entry + SlotHeaderBytes);
		}

		static inline ObjectInstance* PayloadRead(const std::byte* entry)
		{
			ObjectInstance* payload;
			std::memcpy(&payload, entry + SlotHeaderBytes, sizeof(payload));
			return payload;
		}

		void InitializeArena(std::size_t localsCount, std::size_t evalCapacity);
		void GrowArena(std::size_t newLocalCapacity, std::size_t newEvalCapacity);
	};
}
