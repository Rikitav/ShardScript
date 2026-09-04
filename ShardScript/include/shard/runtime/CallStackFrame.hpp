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
//     [CallStackFrame object][return slot][locals region][view arena][eval region]
//     ^-- this               ^-- Arena = this + sizeof(CallStackFrame)
//
// `sizeof` covers the complete object including tail padding, so the arena
// base does not depend on member declaration order. The frame is freed when
// the shared_ptr dies (normally at PopFrame; async frames with pending tasks
// survive via PendingTaskCount until the tasks release them).
//
// Every slot in the regions is a TAGGED, HEADER-DRIVEN byte entry:
//
//     [TypeShape* header][payload ...]
//
// The header encodes BOTH the value's concrete TypeShape (the
// per-instantiation layout metadata — see TypeShape/TypeShapeCache) and the
// payload storage kind, in the pointer's low bit (shapes are at least
// alignof(TypeShape*)-aligned, so the bit is free):
//
//     header & 1 == 1   BOXED reference — payload = ObjectInstance*
//     header & 1 == 0   INLINE value    — payload = byte[AlignUp(shape->Size)]
//
// so EntryStride is derived from the header alone:
//
//     boxed (or shapeless null) -> sizeof(TypeShape*) + sizeof(ObjectInstance*)
//     inline                    -> sizeof(TypeShape*) + AlignUp(shape->Size, 8)
//
// A null value is the boxed form with a null shape (header == BoxedTag).
//
// OWNERSHIP. A slot OWNS whatever references its payload points at:
//   - boxed slot: owns ONE reference to the payload instance (adopt =
//     IncrementReference, release = GarbageCollector::DestroyInstance);
//   - inline slot: owns its REFERENCE FIELDS — the by-value payload bytes are
//     raw data, but any ObjectInstance* stored inside them is co-owned
//     (adopt/release walk the shape's reference slots). Inline payloads are
//     never GC objects themselves: no box, no GcHeader, nothing to free.
// SetLocal/DrainReferences implement exactly this protocol; eval entries and
// locals share it, which is what lets the collect-after-opcode calls in the
// interpreter disappear.
//
// ----------------------------------------------------------------------------
// THE REGIONS
//
//   Return slot   — one reserved entry at the arena base for the method's
//                   return value, laid out like any other slot entry: a
//                   value-type return gets an inline entry sized from the
//                   resolved return shape, a reference/void return keeps the
//                   boxed pointer-sized stride. The VM pre-sets the entry
//                   header before an external callback runs; foreign code
//                   writes the payload (see CallState::WriteReturn /
//                   PlaceReturned / ReturnView).
//
//   Locals region — argument slots followed by local-variable slots. Slots
//                   have PER-SLOT strides: each slot's kind and shape are
//                   resolved ONCE, at Create, from MethodSymbol::FrameLayout
//                   (parameters' types + the FrameSlotRecipe table) and the
//                   method's type arguments, so generic instantiations get
//                   their exact inline layouts (a local of T = BigStruct is
//                   stored inline; T resolved late or unknown stays a
//                   reference slot holding a box). The resolved descriptor
//                   table (shape, byte offset, inline flag) lives on the
//                   frame; the region itself is pre-zeroed, a zeroed entry
//                   reading as a null value / zero inline payload.
//
//   Eval region   — the stack bytecode operands live on. Entries are
//                   VARIABLE-STRIDE: the emitter does not record a static type
//                   per eval slot, so the frame keeps an offsets index
//                   (EvalOffsets, one uint32 byte-offset per live entry) and
//                   Push*/Pop* move a byte cursor over the region. Popping is
//                   therefore safe for any mix of boxed and inline entries.
//
// External methods receive their arguments as an ArgumentsSpan of
// ObjectInstance* — the VM copies argument payloads into a scratch buffer
// per external call (reference payloads verbatim, inline payloads wrapped in
// transient borrow-view ObjectInstance values placed in caller-provided
// storage), so dependent libraries keep the contiguous span ABI they were
// built against. No view header is ever allocated from the frame: an
// ObjectInstance is a lightweight view struct, and wherever legacy code needs
// one over frame bytes the caller supplies the (stack) storage.
//
// GROWTH NEVER MOVES THE FRAME OBJECT. Raw CallStackFrame* pointers are all
// over the runtime — PreviousFrame chains, async task bindings, CurrentFrame
// results — so realloc'ing the single block in place is forbidden. The locals
// region never grows (slot indices are emission-assigned and exact). If the
// eval region outgrows its initial capacity, GrowEvalRegion migrates the arena
// to a separate side block (copying the fixed prefix and live eval bytes,
// flipping ArenaIsTrailing) and the frame object stays where it was. The
// destructor frees the arena only when it is such a side allocation; a
// trailing arena is freed together with the object by the Create() deleter.
//
// ----------------------------------------------------------------------------
// WHERE THE CAPACITY COMES FROM
//
// The arena is sized exactly at frame creation from metadata computed
// during bytecode emission (see MethodSymbol::FrameLayout and the
// EvalLayoutTracker instrumentation in AbstractEmiter / AsyncEmissionPass):
//
//   locals bytes — the sum of the resolved per-slot entry strides (see
//       above). Slot indices are assigned once, during semantic analysis and
//       emission, so the table is exact.
//
//   eval bytes   = max(FrameLayout::MaxEvalDepth, 8) * evalEntryStride
//                = 64 * 16                                        otherwise
//       where evalEntryStride = sizeof(TypeShape*) +
//                               AlignUp(max(sizeof(void*),
//                                         FrameLayout::EvalSlotPayload), 8).
//       MaxEvalDepth is the peak number of eval entries the emitted
//       bytecode can hold and EvalSlotPayload the largest inline payload any
//       push carries, both tracked instruction-by-instruction at emission
//       time; the product is an upper bound on the live eval bytes (an entry
//       is charged for the largest payload even when it holds a pointer).
//       The depth floor of 8 guarantees exception dispatch always has room
//       to push the thrown exception. When the layout is unknown (tracker
//       desync / poisoned, or an external stub with no bytecode) the frame
//       starts with a default region and grows on demand — the only path
//       that can trigger a side migration.
//
// ============================================================================

namespace shard
{
	class VirtualMachine;
	class GarbageCollector;

	enum class FrameInterruptionReason
	{
		None,
		ValueReturned,
		ExceptionRaised,
	};

	struct SHARD_API StackValue
	{
		TypeShape* Shape = nullptr;
		bool IsInline = false;
		std::byte* Data = nullptr;

		inline ObjectInstance* AsObject() const
		{
			return IsInline ? nullptr : *reinterpret_cast<ObjectInstance**>(Data);
		}
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

		static std::shared_ptr<CallStackFrame> Create(const VirtualMachine* host, CallStackFrame* previousFrame, MethodSymbol* method, const std::vector<TypeSymbol*>& typeArguments);

		static constexpr std::uintptr_t BoxedTag = 1;
		static constexpr std::size_t SlotHeaderBytes = sizeof(TypeShape*);
		static constexpr std::size_t ReferencePayloadBytes = sizeof(ObjectInstance*);
		static constexpr std::size_t BoxedEntryStride = SlotHeaderBytes + ReferencePayloadBytes;

		static constexpr std::size_t Align(std::size_t value) { return (value + sizeof(void*) - 1) & ~(sizeof(void*) - 1); }

		static inline bool EntryIsBoxed(const std::byte* entry)
		{
			std::uintptr_t header;
			std::memcpy(&header, entry, sizeof(header));
			return (header & BoxedTag) != 0;
		}

		static inline TypeShape* EntryShape(const std::byte* entry)
		{
			std::uintptr_t header;
			std::memcpy(&header, entry, sizeof(header));
			return reinterpret_cast<TypeShape*>(header & ~BoxedTag);
		}

		static inline std::size_t EntryPayloadBytes(TypeShape* shape)
		{
			return shape != nullptr && !shape->IsReferenceType() ? Align(shape->Size) : ReferencePayloadBytes;
		}

		static inline std::size_t EntryStride(const std::byte* entry)
		{
			return SlotHeaderBytes + EntryPayloadBytes(EntryShape(entry));
		}

		void PushReference(ObjectInstance* value);
		void PushInline(TypeShape* shape, const void* payloadBytes);
		inline void PushStack(ObjectInstance* value) { PushReference(value); }
		std::byte* PushInlineUninitialized(TypeShape* shape);

		ObjectInstance* PopStack();
		ObjectInstance* PeekStack();
		ObjectInstance* PopBoxed(GarbageCollector& gc);

		StackValue TopValue();
		StackValue PopValue();

		void PushCopy(const StackValue& value);

		static void ReleaseValue(const StackValue& value, GarbageCollector& gc);
		static void DiscardValue(const StackValue& value, GarbageCollector& gc);

		inline std::size_t EvalCount() const { return EvalSize; }

		// Inline locals have no ObjectInstance of their own — the storage
		// overload fills caller-provided memory with a borrow view (same
		// contract as ObjectInstance::GetField).
		ObjectInstance* GetLocal(std::uint16_t slot, ObjectInstance& storage);
		StackValue GetLocalValue(std::uint16_t slot);
		ObjectInstance*& LocalRef(std::uint16_t slot);
		void SetLocal(std::uint16_t slot, const StackValue& value, GarbageCollector& gc);

		inline std::size_t LocalCount() const { return LocalSlots.size(); }

		// `storage` must have room for `count` ObjectInstance values; inline
		// argument payloads are placement-new'd into it as borrow views.
		void CopyArgumentPayloads(ObjectInstance** dst, ObjectInstance* storage, std::size_t count);

		// Resolved once at Create from the method's return type: non-null shape
		// means the return slot is an inline entry of Align(shape->Size) payload
		// bytes; null means a boxed (pointer-sized) entry.
		inline TypeShape* ReturnShape() const { return ReturnSlotShape; }
		inline std::byte* ReturnSlotMemory() const { return ReturnSlot; }

		void DrainReferences(GarbageCollector& gc);
		void DrainEvalReferences(GarbageCollector& gc);
		void DrainLocalReferences(GarbageCollector& gc);

		~CallStackFrame();

	private:
		struct LocalSlotDesc
		{
			TypeShape* Shape;
			std::uint32_t Offset;
			bool Inline;
		};

		std::byte* ReturnSlot = nullptr;
		TypeShape* ReturnSlotShape = nullptr;
		std::vector<LocalSlotDesc> LocalSlots;
		std::byte* LocalRegionEnd = nullptr;

		std::byte* EvalEntries = nullptr;
		std::size_t EvalCapacityBytes = 0;
		std::size_t EvalCursorBytes = 0;
		std::size_t EvalSize = 0;
		std::size_t EvalMaxEntryStride = BoxedEntryStride;
		std::vector<std::uint32_t> EvalOffsets;
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

		void GrowEvalRegion(std::size_t newCapacityBytes);
	};
}
