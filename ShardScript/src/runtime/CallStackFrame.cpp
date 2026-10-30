#include <shard/runtime/CallStackFrame.hpp>

#include <shard/pragmatic/FrameLayout.hpp>
#include <shard/pragmatic/PragmaticModel.hpp>
#include <shard/pragmatic/TypeShape.hpp>

#include <shard/semantic/SemanticModel.hpp>
#include <shard/semantic/symbols/MethodSymbol.hpp>
#include <shard/semantic/symbols/StructSymbol.hpp>

#include <gmt/Alignment.hpp>
#include <gmt/Allocator.hpp>
#include <gmt/Containers.hpp>

#include <algorithm>
#include <bit>
#include <stdexcept>

using namespace shard;

std::size_t FrameBlockPool::class_of(std::size_t bytes)
{
	if (bytes <= 16)
		return 4;

	const std::size_t exponent = std::bit_width(bytes - 1);
	return exponent < MaxClasses ? exponent : MaxClasses - 1;
}

std::byte* FrameBlockPool::acquire(std::size_t bytes)
{
	const std::size_t cls = class_of(bytes);
	std::byte* block = m_heads[cls];
	if (block != nullptr)
	{
		m_heads[cls] = *reinterpret_cast<std::byte**>(block);
		return block;
	}

	// the frame header's hot fields are laid out to fit one cache line
	return static_cast<std::byte*>(gmt::mem_alloc_aligned(std::size_t{ 1 } << cls, 64));
}

void FrameBlockPool::release(std::byte* block, std::size_t bytes)
{
	if (block == nullptr)
		return;

	const std::size_t cls = class_of(bytes);
	*reinterpret_cast<std::byte**>(block) = m_heads[cls];
	m_heads[cls] = block;
}

void FrameBlockPool::release_all()
{
	for (std::size_t cls = 0; cls < MaxClasses; ++cls)
	{
		std::byte* block = m_heads[cls];
		while (block != nullptr)
		{
			std::byte* next = *reinterpret_cast<std::byte**>(block);
			gmt::mem_free(block);
			block = next;
		}

		m_heads[cls] = nullptr;
	}
}

FrameBlockPool::~FrameBlockPool()
{
	release_all();
}

std::uintptr_t CallStackFrame::pack_header(const TypeShape* shape, bool isReferenceStorage)
{
	std::uintptr_t header = reinterpret_cast<std::uintptr_t>(shape);
	if (isReferenceStorage)
		header |= ReferenceTag;

	return header;
}

const TypeShape* CallStackFrame::unpack_shape(std::uintptr_t header)
{
	return reinterpret_cast<const TypeShape*>(header & ~ReferenceTag);
}

bool CallStackFrame::unpack_is_reference(std::uintptr_t header)
{
	return (header & ReferenceTag) != 0;
}

std::byte* CallStackFrame::local_entry(std::uint16_t slot) const
{
	return m_localSlots[slot].address;
}

FrameEntryView CallStackFrame::entry_view(std::byte* entry) const
{
	return peek_entry(entry);
}

FrameEntryView CallStackFrame::peek_entry(std::byte* entry)
{
	std::uintptr_t header;
	std::memcpy(&header, entry, EntryHeaderSize);

	FrameEntryView view{};
	view.shape = unpack_shape(header);
	view.isReferenceStorage = unpack_is_reference(header);
	view.payloadAddress = entry + EntryHeaderSize;
	return view;
}

CallStackFrame* CallStackFrame::create(MethodSymbol* method, const FrameLayout* layout, SemanticModel& model, PragmaticModel& pragmatic, CallStackFrame* previous, std::byte* externalReturnEntry, std::size_t externalReturnPayload, FrameBlockPool* pool)
{
	const TypeShape* returnShape = nullptr;
	if (layout != nullptr && layout->has_return_shape())
	{
		returnShape = layout->get_return_shape();
	}
	else
	{
		if (!(method == nullptr) && !(method->get_return_type() == nullptr))
			returnShape = pragmatic.find_or_create_type_shape(model, method->get_return_type());

		if (layout != nullptr)
			layout->set_return_shape(returnShape);
	}

	bool returnReference = returnShape != nullptr && returnShape->is_reference();
	std::size_t returnPayload = 0;
	if (returnShape != nullptr)
		returnPayload = returnReference ? sizeof(void*) : returnShape->get_size();

	const std::size_t returnEntryBytes = EntryHeaderSize + gmt::align_up(returnPayload, sizeof(void*));

	const std::uint16_t argumentCount = layout != nullptr ? layout->get_argument_count() : 0;
	const std::uint16_t variableCount = layout != nullptr ? layout->get_variable_count() : 0;
	const std::uint16_t localCount = static_cast<std::uint16_t>(argumentCount + variableCount);

	// the layout pass resolved these once; fall back to the pragmatic cache
	// when a layout predates the cached lists
	const bool cachedShapes = layout != nullptr
		&& layout->get_argument_shape_count() == argumentCount
		&& layout->get_variable_shape_count() == variableCount;

	auto local_shape = [&](std::size_t index) -> const TypeShape*
	{
		if (cachedShapes)
		{
			if (index < argumentCount)
				return layout->get_argument_shape(index);
			return layout->get_variable_shape(index - argumentCount);
		}

		StructSymbol* type = nullptr;
		if (layout != nullptr)
		{
			type = index < argumentCount
				? layout->get_argument_types()[index]
				: layout->get_variable_slots()[index - argumentCount].concreteType;
		}

		if (type == nullptr)
			return nullptr;

		return pragmatic.find_or_create_type_shape(model, type);
	};

	auto local_reference = [&](std::size_t index, const TypeShape* shape) -> bool
	{
		if (index < argumentCount)
			return shape == nullptr || shape->is_reference();

		// Primitives are always stored inline, whatever the escape verdict;
		// Reference storage is meaningful for composites only.
		// A zero-size reference shape (string, delegates, interfaces) names a variable-size heap object that can never be inlined: it is pointer-stored whatever the verdict
		const FrameSlotRecipe& recipe = layout->get_variable_slots()[index - argumentCount];
		bool reference = recipe.typeParameterIndex >= 0 || recipe.verdict == PlacementVerdict::Heap;
		if (shape != nullptr && !shape->is_reference())
			reference = false;

		if (shape != nullptr && shape->is_reference() && shape->get_size() == 0)
			reference = true;

		return reference;
	};

	auto local_payload = [&](std::size_t index, const TypeShape* shape, bool reference) -> std::size_t
	{
		if (index < argumentCount)
			return sizeof(void*);

		return !reference && shape != nullptr ? shape->get_size() : sizeof(void*);
	};

	std::size_t localsBytes = 0;
	for (std::size_t i = 0; i < localCount; ++i)
	{
		const TypeShape* shape = local_shape(i);
		const bool reference = local_reference(i, shape);
		localsBytes += EntryHeaderSize + gmt::align_up(local_payload(i, shape, reference), sizeof(void*));
	}

	std::size_t evalSlotPayload = layout != nullptr ? layout->get_eval_slot_payload() : 0;
	std::size_t evalStride = EntryHeaderSize + gmt::align_up((std::max)(evalSlotPayload, sizeof(void*)), sizeof(void*));

	std::size_t evalCapacity = 64;
	if (layout != nullptr && layout->is_complete())
		evalCapacity = (std::max)(static_cast<std::size_t>(layout->get_max_eval_depth()), static_cast<std::size_t>(8));

	const std::size_t objectBytes = gmt::align_up(sizeof(CallStackFrame), sizeof(void*));
	const std::size_t metadataBytes = static_cast<std::size_t>(localCount) * sizeof(CallStackFrame::LocalSlot);
	const std::size_t totalBytes = objectBytes + returnEntryBytes + metadataBytes + localsBytes + evalStride * evalCapacity;

	std::byte* block = pool != nullptr
		? pool->acquire(totalBytes)
		: static_cast<std::byte*>(gmt::mem_alloc(totalBytes));
	CallStackFrame* frame = new (block) CallStackFrame();

	frame->m_blockBytes = totalBytes;
	frame->m_layout = layout;
	frame->m_method = method;
	frame->m_previous = previous;
	frame->m_argumentCount = argumentCount;
	frame->m_variableCount = variableCount;
	frame->m_localCount = localCount;

	// payload and address tables live inside the frame block: on the old
	// design every call allocated two more vectors just to describe locals
	frame->m_localSlots = reinterpret_cast<LocalSlot*>(block + objectBytes + returnEntryBytes);
	frame->m_localsRegion = reinterpret_cast<std::byte*>(frame->m_localSlots + localCount);

	frame->m_evalRegion = frame->m_localsRegion + localsBytes;
	frame->m_evalStride = evalStride;
	frame->m_evalCapacity = static_cast<std::uint32_t>(evalCapacity);

	// when the caller chains its return storage in, StackReturn-verdict locals
	// map straight into it: the callee constructs the return value directly
	// in the caller's frame with no copy at all
	bool chained = externalReturnEntry != nullptr;
	if (chained)
	{
		frame->m_returnSlot = externalReturnEntry;
		frame->m_returnPayloadBytes = externalReturnPayload;
		frame->m_hasExternalReturnSlot = true;
	}
	else
	{
		frame->m_returnSlot = block + objectBytes;
		frame->m_returnPayloadBytes = returnPayload;
	}

	std::size_t cursor = 0;
	for (std::size_t i = 0; i < localCount; ++i)
	{
		const TypeShape* shape = local_shape(i);
		const bool reference = local_reference(i, shape);
		const std::size_t payload = local_payload(i, shape, reference);

		LocalSlot& localSlot = frame->m_localSlots[i];
		localSlot.payload = payload;

		bool mappable = chained && i >= argumentCount;
		if (mappable)
		{
			const FrameSlotRecipe& recipe = layout->get_variable_slots()[i - argumentCount];
			mappable = recipe.verdict == PlacementVerdict::StackReturn && shape != nullptr && shape->is_reference();
		}

		std::byte* address = mappable ? frame->m_returnSlot : frame->m_localsRegion + cursor;
		localSlot.address = address;

		cursor += EntryHeaderSize + gmt::align_up(payload, sizeof(void*));

		if (mappable)
			continue;

		std::uintptr_t header = pack_header(shape, reference);
		std::memcpy(address, &header, EntryHeaderSize);
	}

	if (!chained)
	{
		std::uintptr_t returnHeader = pack_header(returnShape, returnReference);
		std::memcpy(frame->m_returnSlot, &returnHeader, EntryHeaderSize);
	}

	return frame;
}

void CallStackFrame::destroy(CallStackFrame* frame, FrameBlockPool* pool)
{
	if (frame == nullptr)
		return;

	const std::size_t bytes = frame->m_blockBytes;
	frame->~CallStackFrame();

	if (pool != nullptr)
		pool->release(reinterpret_cast<std::byte*>(frame), bytes);
	else
		gmt::mem_free(frame);
}

MethodSymbol* CallStackFrame::get_method() const
{
	return m_method;
}

CallStackFrame* CallStackFrame::get_previous() const
{
	return m_previous;
}

std::size_t CallStackFrame::get_instruction_pointer() const
{
	return m_instructionPointer;
}

void CallStackFrame::set_instruction_pointer(std::size_t offset)
{
	m_instructionPointer = offset;
}

InterruptionReason CallStackFrame::get_interruption() const
{
	return m_interruption;
}

void CallStackFrame::set_interruption(InterruptionReason reason)
{
	m_interruption = reason;
}

ObjectInstance CallStackFrame::get_current_exception() const
{
	return m_currentException;
}

void CallStackFrame::set_current_exception(ObjectInstance value)
{
	m_currentException = value;
}

gmt::vector<std::size_t>& CallStackFrame::get_defer_stack()
{
	return m_deferStack;
}

gmt::vector<ExceptionHandlerRecord>& CallStackFrame::get_exception_handlers()
{
	return m_exceptionHandlers;
}

std::uint16_t CallStackFrame::get_argument_count() const
{
	return m_argumentCount;
}

std::uint16_t CallStackFrame::get_variable_count() const
{
	return m_variableCount;
}

std::uint32_t CallStackFrame::get_pending_children() const
{
	return m_pendingChildren;
}

void CallStackFrame::increment_pending_children()
{
	++m_pendingChildren;
}

void CallStackFrame::decrement_pending_children()
{
	if (m_pendingChildren > 0)
		--m_pendingChildren;
}

std::byte* CallStackFrame::get_async_state() const
{
	return m_asyncState;
}

void CallStackFrame::set_async_state(std::byte* state)
{
	m_asyncState = state;
}

std::byte* CallStackFrame::get_awaited_state() const
{
	return m_awaitedState;
}

void CallStackFrame::set_awaited_state(std::byte* state)
{
	m_awaitedState = state;
}

bool CallStackFrame::get_suspended_at_return() const
{
	return m_suspendedAtReturn;
}

void CallStackFrame::set_suspended_at_return(bool value)
{
	m_suspendedAtReturn = value;
}

bool CallStackFrame::get_is_runnable() const
{
	return m_runnable;
}

void CallStackFrame::set_is_runnable(bool value)
{
	m_runnable = value;
}

void CallStackFrame::push_entry(const TypeShape* shape, bool isReferenceStorage, const void* payload, std::size_t payloadBytes)
{
	if (m_evalDepth >= m_evalCapacity)
		throw std::overflow_error("eval stack overflow");

	std::byte* entry = m_evalRegion + m_evalStride * m_evalDepth;
	std::uintptr_t header = pack_header(shape, isReferenceStorage);
	std::memcpy(entry, &header, EntryHeaderSize);

	if (payloadBytes > 0)
		std::memcpy(entry + EntryHeaderSize, payload, payloadBytes);

	++m_evalDepth;
}

ObjectInstance CallStackFrame::pop_entry()
{
	if (m_evalDepth == 0)
		throw std::underflow_error("eval stack underflow");

	--m_evalDepth;
	std::byte* entry = m_evalRegion + m_evalStride * m_evalDepth;

	FrameEntryView view = entry_view(entry);
	if (view.isReferenceStorage)
	{
		std::byte* pointer;
		std::memcpy(&pointer, view.payloadAddress, sizeof(std::byte*));
		return ObjectInstance(view.shape, pointer);
	}

	return ObjectInstance(view.shape, view.payloadAddress);
}

ObjectInstance CallStackFrame::peek_entry() const
{
	if (m_evalDepth == 0)
		throw std::underflow_error("eval stack underflow");

	std::byte* entry = m_evalRegion + m_evalStride * (static_cast<std::size_t>(m_evalDepth) - 1);

	FrameEntryView view = entry_view(entry);
	if (view.isReferenceStorage)
	{
		std::byte* pointer;
		std::memcpy(&pointer, view.payloadAddress, sizeof(std::byte*));
		return ObjectInstance(view.shape, pointer);
	}

	return ObjectInstance(view.shape, view.payloadAddress);
}

std::size_t CallStackFrame::get_eval_depth() const
{
	return m_evalDepth;
}

void CallStackFrame::reset_eval_depth(std::size_t depth)
{
	m_evalDepth = static_cast<std::uint32_t>(depth);
}

ObjectInstance CallStackFrame::peek_entry_at(std::size_t depthFromTop) const
{
	if (depthFromTop >= m_evalDepth)
		throw std::underflow_error("eval stack underflow");

	std::byte* entry = m_evalRegion + m_evalStride * (static_cast<std::size_t>(m_evalDepth) - 1 - depthFromTop);

	FrameEntryView view = entry_view(entry);
	if (view.isReferenceStorage)
	{
		std::byte* pointer;
		std::memcpy(&pointer, view.payloadAddress, sizeof(std::byte*));
		return ObjectInstance(view.shape, pointer);
	}

	return ObjectInstance(view.shape, view.payloadAddress);
}

ObjectInstance CallStackFrame::get_local(std::uint16_t slot) const
{
	FrameEntryView view = entry_view(local_entry(slot));
	if (view.isReferenceStorage)
	{
		std::byte* pointer;
		std::memcpy(&pointer, view.payloadAddress, sizeof(std::byte*));
		return ObjectInstance(view.shape, pointer);
	}

	return ObjectInstance(view.shape, view.payloadAddress);
}

void CallStackFrame::set_local(std::uint16_t slot, ObjectInstance value)
{
	FrameEntryView view = entry_view(local_entry(slot));
	if (view.isReferenceStorage)
	{
		std::byte* pointer = value.get_memory();
		std::memcpy(view.payloadAddress, &pointer, sizeof(std::byte*));
		return;
	}

	std::memcpy(view.payloadAddress, value.get_memory(), m_localSlots[slot].payload);
}

void CallStackFrame::push_local(std::uint16_t slot)
{
	FrameEntryView view = entry_view(local_entry(slot));
	if (view.isReferenceStorage)
	{
		push_entry(view.shape, true, view.payloadAddress, sizeof(std::byte*));
		return;
	}

	if (view.shape != nullptr && view.shape->is_reference())
	{
		std::byte* address = view.payloadAddress;
		push_entry(view.shape, true, &address, sizeof(std::byte*));
		return;
	}

	push_entry(view.shape, false, view.payloadAddress, m_localSlots[slot].payload);
}

ObjectInstance CallStackFrame::get_local_address(std::uint16_t slot) const
{
	FrameEntryView view = entry_view(local_entry(slot));
	return ObjectInstance(view.shape, view.payloadAddress);
}

FrameEntryView CallStackFrame::get_local_view(std::uint16_t slot) const
{
	return entry_view(local_entry(slot));
}

std::byte* CallStackFrame::get_local_entry(std::uint16_t slot) const
{
	return local_entry(slot);
}

FrameEntryView CallStackFrame::get_return_view() const
{
	return entry_view(m_returnSlot);
}

ObjectInstance CallStackFrame::get_return_slot() const
{
	FrameEntryView view = entry_view(m_returnSlot);
	if (view.isReferenceStorage)
	{
		std::byte* pointer;
		std::memcpy(&pointer, view.payloadAddress, sizeof(std::byte*));
		return ObjectInstance(view.shape, pointer);
	}

	return ObjectInstance(view.shape, view.payloadAddress);
}

std::byte* CallStackFrame::get_return_entry() const
{
	return m_returnSlot;
}

std::size_t CallStackFrame::get_return_payload_bytes() const
{
	return m_returnPayloadBytes;
}

bool CallStackFrame::has_external_return_slot() const
{
	return m_hasExternalReturnSlot;
}

