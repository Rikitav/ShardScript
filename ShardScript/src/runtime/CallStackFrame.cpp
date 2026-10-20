#include <shard/runtime/CallStackFrame.hpp>

#include <shard/pragmatic/FrameLayout.hpp>
#include <shard/pragmatic/PragmaticModel.hpp>
#include <shard/pragmatic/TypeShape.hpp>

#include <shard/semantic/SemanticModel.hpp>
#include <shard/semantic/symbols/MethodSymbol.hpp>
#include <shard/semantic/symbols/StructSymbol.hpp>

#include <gmt/Alignment.hpp>

#include <mimalloc.h>

#include <algorithm>
#include <cstring>
#include <stdexcept>

using namespace shard;

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
	return m_localAddresses[slot];
}

FrameEntryView CallStackFrame::entry_view(std::byte* entry) const
{
	return peek_entry(entry);
}

FrameEntryView CallStackFrame::peek_entry(std::byte* entry)
{
	std::uintptr_t header;
	std::memcpy(&header, entry, EntryHeaderSize);

	FrameEntryView view;
	view.shape = unpack_shape(header);
	view.isReferenceStorage = unpack_is_reference(header);
	view.payloadAddress = entry + EntryHeaderSize;
	return view;
}

CallStackFrame* CallStackFrame::create(gmt::Ref<MethodSymbol> method, const FrameLayout* layout, SemanticModel& model, PragmaticModel& pragmatic, CallStackFrame* previous, std::byte* externalReturnEntry, std::size_t externalReturnPayload)
{
	const TypeShape* returnShape = nullptr;
	if (!method.is_null() && !method->get_return_type().is_null())
		returnShape = pragmatic.find_or_create_type_shape(model, method->get_return_type());

	bool returnReference = returnShape != nullptr && returnShape->is_reference();
	std::size_t returnPayload = 0;
	if (returnShape != nullptr)
		returnPayload = returnReference ? sizeof(void*) : returnShape->get_size();

	const std::size_t returnEntryBytes = EntryHeaderSize + gmt::align_up(returnPayload, sizeof(void*));

	std::uint16_t argumentCount = layout != nullptr ? layout->get_argument_count() : 0;
	std::uint16_t variableCount = layout != nullptr ? layout->get_variable_count() : 0;

	std::vector<std::byte*> localAddresses;
	std::vector<std::size_t> localPayloads;
	std::vector<const TypeShape*> localShapes;
	std::vector<bool> localReferences;

	localAddresses.reserve(argumentCount + variableCount);
	localPayloads.reserve(argumentCount + variableCount);
	localShapes.reserve(argumentCount + variableCount);
	localReferences.reserve(argumentCount + variableCount);

	std::size_t cursor = 0;
	if (layout != nullptr)
	{
		for (gmt::Ref<StructSymbol> argumentType : layout->get_argument_types())
		{
			const TypeShape* shape = nullptr;
			if (!argumentType.is_null())
				shape = pragmatic.find_or_create_type_shape(model, argumentType);

			localAddresses.push_back(nullptr);
			localPayloads.push_back(sizeof(void*));
			localShapes.push_back(shape);
			localReferences.push_back(shape == nullptr || shape->is_reference());
			cursor += EntryHeaderSize + gmt::align_up(sizeof(void*), sizeof(void*));
		}

		for (const FrameSlotRecipe& recipe : layout->get_variable_slots())
		{
			const TypeShape* shape = nullptr;
			if (!recipe.concreteType.is_null())
				shape = pragmatic.find_or_create_type_shape(model, recipe.concreteType);

			// primitives are always stored inline, whatever the escape verdict;
			// reference storage is meaningful for composites only. A zero-size
			// reference shape (string, delegates, interfaces) names a
			// variable-size heap object that can never be inlined: it is
			// pointer-stored whatever the verdict
			bool reference = recipe.typeParameterIndex >= 0 || recipe.verdict == PlacementVerdict::Heap;
			if (shape != nullptr && !shape->is_reference())
				reference = false;

			if (shape != nullptr && shape->is_reference() && shape->get_size() == 0)
				reference = true;

			std::size_t payload = sizeof(void*);
			if (!reference && shape != nullptr)
				payload = shape->get_size();

			localAddresses.push_back(nullptr);
			localPayloads.push_back(payload);
			localShapes.push_back(shape);
			localReferences.push_back(reference);
			cursor += EntryHeaderSize + gmt::align_up(payload, sizeof(void*));
		}
	}

	std::size_t evalSlotPayload = layout != nullptr ? layout->get_eval_slot_payload() : 0;
	std::size_t evalStride = EntryHeaderSize + gmt::align_up((std::max)(evalSlotPayload, sizeof(void*)), sizeof(void*));

	std::size_t evalCapacity = 64;
	if (layout != nullptr && layout->is_complete())
		evalCapacity = (std::max)(static_cast<std::size_t>(layout->get_max_eval_depth()), static_cast<std::size_t>(8));

	const std::size_t objectBytes = gmt::align_up(sizeof(CallStackFrame), sizeof(void*));
	const std::size_t totalBytes = objectBytes + returnEntryBytes + cursor + evalStride * evalCapacity;

	std::byte* block = static_cast<std::byte*>(mi_zalloc(totalBytes));
	CallStackFrame* frame = new (block) CallStackFrame();

	frame->m_method = method;
	frame->m_previous = previous;
	frame->m_argumentCount = argumentCount;
	frame->m_variableCount = variableCount;

	frame->m_localsRegion = block + objectBytes + returnEntryBytes;
	frame->m_localPayloads = std::move(localPayloads);

	frame->m_evalRegion = frame->m_localsRegion + cursor;
	frame->m_evalStride = evalStride;
	frame->m_evalCapacity = evalCapacity;

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

	cursor = 0;
	std::size_t recipeIndex = 0;
	for (std::size_t i = 0; i < localShapes.size(); ++i)
	{
		bool mappable = chained && i >= argumentCount;
		if (mappable)
		{
			const FrameSlotRecipe& recipe = layout->get_variable_slots()[recipeIndex++];
			mappable = recipe.verdict == PlacementVerdict::StackReturn && localShapes[i] != nullptr && localShapes[i]->is_reference();
		}

		if (mappable)
			localAddresses[i] = frame->m_returnSlot;
		else
			localAddresses[i] = frame->m_localsRegion + cursor;

		cursor += EntryHeaderSize + gmt::align_up(frame->m_localPayloads[i], sizeof(void*));

		if (mappable)
			continue;

		std::uintptr_t header = pack_header(localShapes[i], localReferences[i]);
		std::memcpy(localAddresses[i], &header, EntryHeaderSize);
	}

	if (!chained)
	{
		std::uintptr_t returnHeader = pack_header(returnShape, returnReference);
		std::memcpy(frame->m_returnSlot, &returnHeader, EntryHeaderSize);
	}

	frame->m_localAddresses = std::move(localAddresses);
	return frame;
}

void CallStackFrame::destroy(CallStackFrame* frame)
{
	if (frame == nullptr)
		return;

	frame->~CallStackFrame();
	mi_free(frame);
}

gmt::Ref<MethodSymbol> CallStackFrame::get_method() const
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

std::vector<std::size_t>& CallStackFrame::get_defer_stack()
{
	return m_deferStack;
}

std::vector<ExceptionHandlerRecord>& CallStackFrame::get_exception_handlers()
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

	std::byte* entry = m_evalRegion + m_evalStride * (m_evalDepth - 1);

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

	std::byte* entry = m_evalRegion + m_evalStride * (m_evalDepth - 1 - depthFromTop);

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

	std::memcpy(view.payloadAddress, value.get_memory(), m_localPayloads[slot]);
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

	push_entry(view.shape, false, view.payloadAddress, m_localPayloads[slot]);
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

void CallStackFrame::for_each_live_entry(const std::function<void(const FrameEntryView&)>& visitor)
{
	FrameEntryView returnView = entry_view(m_returnSlot);
	if (returnView.shape != nullptr)
		visitor(returnView);

	for (std::size_t i = 0; i < m_localAddresses.size(); ++i)
	{
		FrameEntryView view = entry_view(m_localAddresses[i]);
		if (view.shape != nullptr)
			visitor(view);
	}

	for (std::uint32_t i = 0; i < m_evalDepth; ++i)
	{
		FrameEntryView view = entry_view(m_evalRegion + m_evalStride * i);
		if (view.shape != nullptr)
			visitor(view);
	}
}
