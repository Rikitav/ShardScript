#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/runtime/ObjectInstance.hpp>

#include <shard/semantic/symbols/TypeSymbol.hpp>
#include <shard/semantic/symbols/MethodSymbol.hpp>

#include <cstddef>
#include <cstdint>
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
// The three regions are arrays of ObjectInstance* slots (pointer-sized
// entries) today. Stage 4 item 2 replaces the slot element with a tagged
// header-driven byte entry ([TypeShape*][payload]) inside the same arena;
// the region structure below is the stable part.
//
//   Return slot   — one reserved slot at the arena base, sized for the
//                   method's return value. Unused until Stage 5 wires
//                   foreign-function value returns through it.
//   Locals region — argument slots followed by local-variable slots,
//                   pre-zeroed. Argument transfer on invocation writes
//                   directly into the front of this region, and external
//                   methods receive an ArgumentsSpan over it.
//   Eval region   — the stack bytecode operands live on. PushStack/PopStack
//                   move an EvalSize cursor over it; values above the locals
//                   live and die here.
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
// EvalLayoutTracker instrumentation in AbstractEmiter / AsyncEmissionPass):
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

		void PushStack(ObjectInstance* value);
		ObjectInstance* PopStack();
		ObjectInstance* PeekStack();

		ObjectInstance* GetLocal(std::uint16_t slot) const;
		ObjectInstance*& LocalRef(std::uint16_t slot);

		inline std::size_t EvalCount() const { return EvalSize; }
		inline std::size_t LocalCount() const { return LocalCapacity; }

		ObjectInstance** LocalsData() { return LocalSlots; }

		~CallStackFrame();

	private:
		ObjectInstance** ReturnSlotStorage = nullptr;
		ObjectInstance** LocalSlots = nullptr;
		std::size_t LocalCapacity = 0;
		ObjectInstance** EvalSlots = nullptr;
		std::size_t EvalCapacity = 0;
		std::size_t EvalSize = 0;
		bool ArenaIsTrailing = false;

		void InitializeArena(std::size_t localsCount, std::size_t evalCapacity);
		void GrowArena(std::size_t newLocalCapacity, std::size_t newEvalCapacity);
	};
}
