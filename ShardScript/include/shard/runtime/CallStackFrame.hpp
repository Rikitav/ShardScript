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
// stack.
//
// MEMORY LAYOUT
//
// `CallStackFrame::Create` mi_mallocs ONE fixed-size block:
//
//     [CallStackFrame][return slot][args region][locals region][eval region]
//
// The `CallStackFrame` object is placement-new'd at the block head and the
// arena (args/locals/eval + a single reserved return entry) trails it. The
// block is sized once at creation from the bytecode-emission metadata
// (MethodSymbol::Layout) and is never reallocated. The only runtime sizing is
// for generic methods: when TypeArguments are supplied, substituted type
// shapes are resolved before the mi_malloc and their exact inline sizes fold
// into the arena size.
//
// Every slot in args/locals/eval is a `{ TypeShape*, Payload }` entry:
//   - by-value instance  -> inline payload of Align(shape->Size) bytes;
//   - by-reference       -> payload holds a pointer to a GC-heap-allocated
//                           instance ([GcHeader][payload]); null = null ptr.
//
// Region memory is never wiped or zeroed; slots are only (re)written when a
// value is pushed/stored.
//
// EVAL REGION
//
// The eval region grows/shrinks via a byte cursor. Push advances the cursor by
// sizeof(TypeShape*) + Align(payload size); PopStack rewinds it by the same
// amount and returns an ObjectInstance VIEW into the eval region. Popped
// values are never cleared — they stay in place until the next push overwrites
// them.
// ============================================================================

namespace shard
{
	class VirtualMachine;
	class GarbageCollector;

	enum class FrameInterruptionReason
	{
		None,
		EndOfInstructions,
		ValueReturned,
		ExceptionRaised,
	};

	class SHARD_API CallStackFrame : public std::enable_shared_from_this<CallStackFrame>
	{
		friend class VirtualMachine;
		friend class CallState;

		struct LocalSlotDesc
		{
			const TypeShape* Shape;     // resolved shape (value shape for inline, referenced object shape for references; null if unknown)
			std::uint32_t Offset;		// byte offset into the arena
		};

		std::byte* ReturnSlot = nullptr;
		TypeShape* ReturnSlotShape = nullptr;

		std::vector<LocalSlotDesc> LocalSlots;
		std::byte* LocalRegionEnd = nullptr;

		std::byte* EvalEntries = nullptr;
		std::size_t EvalCapacityBytes = 0;
		std::size_t EvalCursorBytes = 0;
		std::vector<std::uint32_t> EvalOffsets;

	public:
		struct ExceptionHandlerFrame
		{
			std::size_t HandlerOffset;
			std::size_t DeferStackBase;
		};

		static constexpr std::size_t SlotHeaderBytes = sizeof(TypeShape*);
		static constexpr std::size_t ReferencePayloadBytes = sizeof(void*);
		static constexpr std::size_t BoxedEntryStride = SlotHeaderBytes + ReferencePayloadBytes;

		const VirtualMachine* Host;
		const CallStackFrame* PreviousFrame;
		const MethodSymbol* Method;

		std::byte* Arena = nullptr;
		std::size_t ArenaBytes = 0;

		std::vector<TypeSymbol*> TypeArguments;
		std::vector<ExceptionHandlerFrame> ExceptionHandlers;
		std::size_t PendingTaskCount = 0;

		std::vector<std::size_t> DeferStack;
		std::size_t DeferDrainDepth = 0;

		FrameInterruptionReason InterruptionReason = FrameInterruptionReason::None;
		ObjectInstance CurrentException;

	private:
		CallStackFrame(const VirtualMachine* host, const CallStackFrame* previousFrame, const MethodSymbol* method)
			: Host(host), Method(method), PreviousFrame(previousFrame) { }

	public:
		~CallStackFrame();

		static std::shared_ptr<CallStackFrame> Create(
			const VirtualMachine* host,
			const CallStackFrame* previousFrame,
			const MethodSymbol* method,
			const std::vector<TypeSymbol*>& typeArguments);

		void CopyArgumentPayloads();
		TypeSymbol* ResolveType(TypeSymbol* type);

		ObjectInstance GetLocal(std::uint16_t slot);
		void SetLocal(std::uint16_t slot, const ObjectInstance& value);

		ObjectInstance PushStack(ObjectInstance value);
		ObjectInstance PopStack();
		ObjectInstance PeekStack();

		void PlaceReturned(ObjectInstance value);
		ObjectInstance ReturnView() const;

		inline bool interrupted() const
		{
			return InterruptionReason != FrameInterruptionReason::None;
		}

		inline std::size_t LocalCount() const
		{
			return LocalSlots.size();
		}

		inline std::size_t EvalCount() const
		{
			return EvalOffsets.size();
		}

		inline TypeShape* ReturnShape() const
		{
			return ReturnSlotShape;
		}

		inline std::byte* ReturnSlotMemory() const
		{
			return ReturnSlot;
		}

	private:
		ObjectInstance PushReference(ObjectInstance value);
		ObjectInstance PushInline(const TypeShape* shape, const void* payloadBytes);
		ObjectInstance PushInlineUninitialized(const TypeShape* shape);

		void DrainReferences(GarbageCollector& gc);
		void DrainEvalReferences(GarbageCollector& gc);
		void DrainLocalReferences(GarbageCollector& gc);

		static void ReleaseValue(const ObjectInstance& value, GarbageCollector& gc);
		static void DiscardValue(const ObjectInstance& value, GarbageCollector& gc);
	};
}
