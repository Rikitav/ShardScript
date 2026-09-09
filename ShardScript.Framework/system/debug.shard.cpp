#include <ShardScript.hpp>

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace shard;

namespace
{
	constexpr std::size_t AlignSize(std::size_t value)
	{
		return (value + sizeof(void*) - 1) & ~(sizeof(void*) - 1);
	}

	// Mirrors CallStackFrame's entry layout: inline payloads store Align(shape->Size)
	// bytes, reference payloads (and unknown shapes) store a single pointer.
	std::size_t PayloadBytesOf(const TypeShape* shape)
	{
		return shape != nullptr && !shape->IsReferenceType()
			? AlignSize(shape->Size)
			: CallStackFrame::ReferencePayloadBytes;
	}

	std::size_t EntryStrideOf(const TypeShape* shape)
	{
		return CallStackFrame::SlotHeaderBytes + PayloadBytesOf(shape);
	}

	const TypeShape* ReadEntryShape(const std::byte* entry)
	{
		return *reinterpret_cast<const TypeShape* const*>(entry);
	}

	std::wstring ShapeName(const TypeShape* shape)
	{
		if (shape == nullptr || shape->BaseType == nullptr)
			return L"<none>";

		return shape->BaseType->FullName;
	}

	std::wstring PointerHex(const void* ptr)
	{
		wchar_t buffer[24];
		swprintf(buffer, 24, L"0x%p", ptr);
		return buffer;
	}

	std::wstring HexBytes(const std::byte* data, std::size_t count)
	{
		std::wstring result;
		result.reserve(count * 3);

		wchar_t buffer[4];
		for (std::size_t i = 0; i < count; ++i)
		{
			swprintf(buffer, 4, L"%02X ", static_cast<unsigned int>(data[i]));
			result += buffer;
		}

		if (!result.empty() && result.back() == L' ')
			result.pop_back();

		return result;
	}

	// Resolve the frame `depth` levels above the debug callback's own frame:
	// depth 0 is the calling function's frame, depth 1 is its caller, and so on.
	const CallStackFrame* ResolveCallerFrame(const CallState& context, std::int64_t depth)
	{
		const CallStackFrame* frame = context.Frame->PreviousFrame;
		for (std::int64_t i = 0; frame != nullptr && i < depth; ++i)
			frame = frame->PreviousFrame;

		return frame;
	}

	// Payload pointer of a locals-region slot, recomputed purely from raw
	// memory (header + payload), independent of GetLocal().
	std::byte* ReadSlotPayload(std::byte* entry, const TypeShape* shape)
	{
		if (shape != nullptr && !shape->IsReferenceType())
			return entry + CallStackFrame::SlotHeaderBytes;

		return *reinterpret_cast<std::byte**>(entry + CallStackFrame::SlotHeaderBytes);
	}

	// Raw-memory rendering of one slot entry: inline payloads dump their bytes,
	// reference payloads dump the stored pointer and the bytes it points to.
	std::wstring DumpSlotBytes(std::byte* entry, const TypeShape* shape)
	{
		if (shape == nullptr)
			return L"<no shape>";

		if (!shape->IsReferenceType())
			return HexBytes(entry + CallStackFrame::SlotHeaderBytes, std::min<std::size_t>(shape->Size, 16));

		std::byte* target = *reinterpret_cast<std::byte**>(entry + CallStackFrame::SlotHeaderBytes);
		std::wstring result = PointerHex(target);
		if (target != nullptr)
			result += L" -> " + HexBytes(target, 16);

		return result;
	}

	std::wstring FormatLocalSlot(const CallStackFrame* frame, std::size_t index)
	{
		CallStackFrame::LocalSlotInfo info = frame->GetLocalSlotInfo(index);
		std::byte* entry = frame->Arena + info.Offset;
		const TypeShape* headerShape = ReadEntryShape(entry);

		std::wostringstream line;
		line << L"  [" << index << L"] " << (info.IsArgument ? L"arg" : L"local")
			 << L" @" << info.Offset
			 << L" shape='" << ShapeName(info.Shape) << L"'";

		if (headerShape == nullptr)
		{
			line << L" <uninitialized>";
			return line.str();
		}

		if (headerShape != info.Shape)
			line << L" header-shape='" << ShapeName(headerShape) << L"'";

		line << L" " << DumpSlotBytes(entry, headerShape);
		return line.str();
	}

	std::wstring FormatEvalEntry(const CallStackFrame* frame, std::size_t index)
	{
		const std::vector<std::uint32_t>& offsets = frame->GetEvalOffsets();
		std::byte* entry = frame->GetEvalRegionStart() + offsets[index];
		const TypeShape* shape = ReadEntryShape(entry);

		std::wostringstream line;
		line << L"  [" << index << L"] @" << offsets[index];
		if (shape == nullptr)
		{
			line << L" <null literal>";
			return line.str();
		}

		line << L" shape='" << ShapeName(shape) << L"' " << DumpSlotBytes(entry, shape);
		return line.str();
	}

	std::wstring InspectFrameLayout(const CallStackFrame* frame, std::int64_t depth)
	{
		std::wostringstream out;
		out << L"frame #" << depth << L" '"
			<< (frame->Method != nullptr ? frame->Method->FullName : L"<none>") << L"'\n";
		out << L"  arena: " << PointerHex(frame->Arena)
			<< L" (" << frame->ArenaBytes << L" bytes)\n";
		out << L"  return slot: " << PointerHex(frame->ReturnSlotMemory())
			<< L" stride " << frame->GetReturnSlotStride()
			<< L" shape '" << ShapeName(frame->ReturnShape()) << L"'\n";
		out << L"  locals region: [" << PointerHex(frame->GetLocalsRegionStart())
			<< L", " << PointerHex(frame->GetEvalRegionStart()) << L") "
			<< frame->GetArgumentSlotCount() << L" args, "
			<< (frame->GetLocalSlotCount() - frame->GetArgumentSlotCount()) << L" locals\n";

		for (std::size_t i = 0; i < frame->GetLocalSlotCount(); ++i)
			out << FormatLocalSlot(frame, i) << L"\n";

		out << L"  eval region: " << PointerHex(frame->GetEvalRegionStart())
			<< L" used " << frame->GetEvalRegionUsedBytes()
			<< L" / " << frame->GetEvalRegionCapacity()
			<< L" bytes, " << frame->GetEvalOffsets().size() << L" live entries\n";

		for (std::size_t i = 0; i < frame->GetEvalOffsets().size(); ++i)
			out << FormatEvalEntry(frame, i) << L"\n";

		return out.str();
	}

	// Layout invariants over the raw arena bytes. Returns an empty string when
	// the frame is consistent, otherwise one violation per line.
	std::wstring VerifyFrameLayout(const CallStackFrame* frame)
	{
		std::wostringstream violations;

		// 1. The return slot must be the first entry of the arena.
		if (frame->ReturnSlotMemory() != frame->Arena)
			violations << L"return slot is not at arena base\n";

		// 2. The three regions must tile the arena block exactly:
		//    [return slot][args+locals][eval].
		const TypeShape* returnShape = frame->ReturnShape();
		if (returnShape == nullptr)
		{
			violations << L"return shape is null\n";
		}
		else
		{
			const std::size_t expectedStride = CallStackFrame::SlotHeaderBytes + PayloadBytesOf(returnShape);
			if (frame->GetReturnSlotStride() != expectedStride)
				violations << L"return slot stride mismatch\n";
		}

		const std::size_t localsEndOffset = static_cast<std::size_t>(frame->GetEvalRegionStart() - frame->Arena);
		if (frame->GetLocalsRegionStart() > frame->GetEvalRegionStart())
			violations << L"locals region start is past eval region start\n";
		if (frame->GetEvalRegionStart() + frame->GetEvalRegionCapacity() != frame->Arena + frame->ArenaBytes)
			violations << L"arena regions do not tile the arena block\n";

		// 3. Local slots: ascending, non-overlapping, in-bounds, header agrees
		//    with the resolved shape, and the raw-memory payload pointer must
		//    byte-match what GetLocal() reports.
		std::size_t expectedOffset = frame->GetReturnSlotStride();
		for (std::size_t i = 0; i < frame->GetLocalSlotCount(); ++i)
		{
			CallStackFrame::LocalSlotInfo info = frame->GetLocalSlotInfo(i);
			const std::size_t stride = EntryStrideOf(info.Shape);

			if (info.Offset != expectedOffset)
				violations << L"slot " << i << L" offset mismatch (gaps or overlap)\n";
			if (info.Offset + stride > localsEndOffset)
				violations << L"slot " << i << L" exceeds locals region\n";

			std::byte* entry = frame->Arena + info.Offset;
			const TypeShape* headerShape = ReadEntryShape(entry);
			if (headerShape != nullptr && headerShape != info.Shape)
				violations << L"slot " << i << L" header shape differs from resolved shape\n";

			if (headerShape != nullptr &&
				frame->GetLocal(static_cast<std::uint16_t>(i)).getMemory() != ReadSlotPayload(entry, headerShape))
			{
				violations << L"slot " << i << L" raw payload disagrees with GetLocal()\n";
			}

			expectedOffset = info.Offset + stride;
		}

		// 4. Eval entries: ascending offsets, entries within the used bytes,
		//    cursor within capacity.
		const std::vector<std::uint32_t>& offsets = frame->GetEvalOffsets();
		bool first = true;
		std::uint32_t previousOffset = 0;
		for (std::size_t i = 0; i < offsets.size(); ++i)
		{
			if (!first && offsets[i] <= previousOffset)
				violations << L"eval entry " << i << L" is not strictly ascending\n";
			first = false;

			std::byte* entry = frame->GetEvalRegionStart() + offsets[i];
			const std::size_t stride = EntryStrideOf(ReadEntryShape(entry));
			if (static_cast<std::size_t>(offsets[i]) + stride > frame->GetEvalRegionUsedBytes())
				violations << L"eval entry " << i << L" exceeds used eval bytes\n";

			previousOffset = offsets[i];
		}

		if (frame->GetEvalRegionUsedBytes() > frame->GetEvalRegionCapacity())
			violations << L"eval cursor exceeds eval capacity\n";

		std::wstring result = violations.str();
		if (!result.empty() && result.back() == L'\n')
			result.pop_back();

		return result;
	}
}

static void shard_debug_typeof(const CallState& context)
{
	auto [instance] = context.GetArgs<ObjectInstance>();
	if (instance.IsNullInstance())
		throw undefined_behaviour("cannot get type of null instance");

	context.PlaceReturned(context.Collector.FromString(instance.getInfo()->Name));
}

static void shard_debug_sizeof(const CallState& context)
{
	auto [instance] = context.GetArgs<ObjectInstance>();
	if (instance.IsNullInstance())
		throw undefined_behaviour("cannot get size of null instance");

	context.ReturnInteger(static_cast<std::int64_t>(instance.getInfo()->MemoryBytesSize));
}

static void shard_debug_PrintGcInfo(const CallState& context)
{
	std::wcout << L"\nGarbage collector info dump" << std::endl;
	for (std::byte* payload : context.Collector.Heap)
	{
		const ObjectInstance::GcHeader* header =
			reinterpret_cast<const ObjectInstance::GcHeader*>(payload - sizeof(ObjectInstance::GcHeader));

		if (header->Magic != ObjectInstance::GcHeader::MAGIC)
		{
			std::wcout << L" * PTR : " << PointerHex(payload) << L" | <corrupt header>\n";
			continue;
		}

		ObjectInstance instance(header->Shape, payload);
		std::wcout << L" * PTR : " << PointerHex(payload)
				   << L" | TYPE : '" << (instance.getInfo() != nullptr ? instance.getInfo()->Name : L"<none>")
				   << L"' | REFS : " << instance.getReferencesCounter() << L"\n";
	}

	std::wcout << L"Total count : " << context.Collector.Heap.size() << std::endl;
}

static void shard_debug_PrintStackFrameInfo(const CallState& context)
{
	std::wcout << L"\nCall stack frame variables dump :" << std::endl;

	const CallStackFrame* frame = context.Frame->PreviousFrame;
	if (frame == nullptr)
	{
		std::wcout << L"no caller frame" << std::endl;
		return;
	}

	for (std::size_t i = 0; i < frame->GetLocalSlotCount(); ++i)
		std::wcout << FormatLocalSlot(frame, i) << L"\n";

	std::wcout << L"Total count : " << frame->GetLocalSlotCount() << std::endl;
}

static void shard_debug_inspect_stack(const CallState& context)
{
	std::wostringstream out;

	std::int64_t depth = 0;
	for (const CallStackFrame* frame = context.Frame->PreviousFrame; frame != nullptr; frame = frame->PreviousFrame)
	{
		out << L"frame #" << depth++ << L" '"
			<< (frame->Method != nullptr ? frame->Method->FullName : L"<none>") << L"'"
			<< L" arena=" << PointerHex(frame->Arena)
			<< L" (" << frame->ArenaBytes << L" bytes)"
			<< L" args=" << frame->GetArgumentSlotCount()
			<< L" locals=" << (frame->GetLocalSlotCount() - frame->GetArgumentSlotCount())
			<< L" eval-used=" << frame->GetEvalRegionUsedBytes() << L"/" << frame->GetEvalRegionCapacity()
			<< L"\n";
	}

	if (depth == 0)
		out << L"<empty>";

	context.PlaceReturned(context.Collector.FromString(out.str()));
}

static void shard_debug_inspect_frame(const CallState& context)
{
	auto [depth] = context.GetArgs<std::int64_t>();
	const CallStackFrame* frame = ResolveCallerFrame(context, depth);
	if (frame == nullptr)
	{
		context.PlaceReturned(context.Collector.FromString(L"<no frame at depth " + std::to_wstring(depth) + L">"));
		return;
	}

	context.PlaceReturned(context.Collector.FromString(InspectFrameLayout(frame, depth)));
}

static void shard_debug_verify_frame(const CallState& context)
{
	auto [depth] = context.GetArgs<std::int64_t>();
	const CallStackFrame* frame = ResolveCallerFrame(context, depth);
	if (frame == nullptr)
	{
		context.PlaceReturned(context.Collector.FromString(L"no frame at depth " + std::to_wstring(depth)));
		return;
	}

	context.PlaceReturned(context.Collector.FromString(VerifyFrameLayout(frame)));
}

static void shard_debug_frame_arena_address(const CallState& context)
{
	auto [depth] = context.GetArgs<std::int64_t>();
	const CallStackFrame* frame = ResolveCallerFrame(context, depth);
	context.PlaceReturned(context.Collector.FromNint(reinterpret_cast<std::intptr_t>(frame != nullptr ? frame->Arena : nullptr)));
}

static void shard_debug_read_memory(const CallState& context)
{
	// Addresses arrive as nint (NativeInteger); unwrap via the pointer overload,
	// which accepts TYPE_NINT payloads.
	auto [address, bytes] = context.GetArgs<std::byte*, std::int64_t>();
	bytes = std::clamp<std::int64_t>(bytes, 0, 256);

	context.PlaceReturned(context.Collector.FromString(HexBytes(address, static_cast<std::size_t>(bytes))));
}

static void shard_debug_compare_memory(const CallState& context)
{
	auto [first, second, bytes] = context.GetArgs<std::byte*, std::byte*, std::int64_t>();
	bytes = std::clamp<std::int64_t>(bytes, 0, 4096);

	context.ReturnBoolean(std::memcmp(first, second, static_cast<std::size_t>(bytes)) == 0);
}

SHARDLIB_GETMETADATA
{
	lib.Name = L"shard.debug";
	lib.Description = L"ShardScript debug and frame inspection functions";
	lib.Version = L"1.0.0";
}

SHARDLIB_ENTRYPOINT
{
	SymbolBuilder<NamespaceSymbol> debug(context, L"debug");

	debug.AddMethod(L"typeof", TYPE_STRING, LINK_STATIC, ACS_PUBLIC)
		 .AddParameter(L"object", TYPE_ANY)
		 .SetCallback(&shard_debug_typeof);

	debug.AddMethod(L"sizeof", TYPE_INT, LINK_STATIC, ACS_PUBLIC)
		 .AddParameter(L"object", TYPE_ANY)
		 .SetCallback(&shard_debug_sizeof);

	debug.AddMethod(L"PrintGcInfo", TYPE_VOID, LINK_STATIC, ACS_PUBLIC)
		 .SetCallback(&shard_debug_PrintGcInfo);

	debug.AddMethod(L"PrintStackFrameInfo", TYPE_VOID, LINK_STATIC, ACS_PUBLIC)
		 .SetCallback(&shard_debug_PrintStackFrameInfo);

	debug.AddMethod(L"inspectStack", TYPE_STRING, LINK_STATIC, ACS_PUBLIC)
		 .SetCallback(&shard_debug_inspect_stack);

	debug.AddMethod(L"inspectFrame", TYPE_STRING, LINK_STATIC, ACS_PUBLIC)
		 .AddParameter(L"depth", TYPE_INT)
		 .SetCallback(&shard_debug_inspect_frame);

	debug.AddMethod(L"verifyFrame", TYPE_STRING, LINK_STATIC, ACS_PUBLIC)
		 .AddParameter(L"depth", TYPE_INT)
		 .SetCallback(&shard_debug_verify_frame);

	debug.AddMethod(L"frameArenaAddress", SymbolTable::Primitives::NativeInteger, LINK_STATIC, ACS_PUBLIC)
		 .AddParameter(L"depth", TYPE_INT)
		 .SetCallback(&shard_debug_frame_arena_address);

	debug.AddMethod(L"readMemory", TYPE_STRING, LINK_STATIC, ACS_PUBLIC)
		 .AddParameter(L"address", SymbolTable::Primitives::NativeInteger)
		 .AddParameter(L"bytes", TYPE_INT)
		 .SetCallback(&shard_debug_read_memory);

	debug.AddMethod(L"compareMemory", TYPE_BOOL, LINK_STATIC, ACS_PUBLIC)
		 .AddParameter(L"first", SymbolTable::Primitives::NativeInteger)
		 .AddParameter(L"second", SymbolTable::Primitives::NativeInteger)
		 .AddParameter(L"bytes", TYPE_INT)
		 .SetCallback(&shard_debug_compare_memory);
}
