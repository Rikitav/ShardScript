#include <shard/runtime/CallStackFrame.hpp>

#include <shard/pragmatic/FrameLayout.hpp>
#include <shard/pragmatic/PragmaticModel.hpp>
#include <shard/pragmatic/TypeShape.hpp>
#include <shard/runtime/GarbageCollector.hpp>

#include <shard/parsing/SyntaxKind.hpp>
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

CallStackFrame* CallStackFrame::create(MethodSymbol* method, const FrameLayout* layout, SemanticModel& model, PragmaticModel& pragmatic, CallStackFrame* previous, GarbageCollector* collector, std::byte* externalReturnDestination, std::size_t externalReturnPayload, FrameBlockPool* pool)
{
	(void)externalReturnPayload;
	const TypeShape* returnShape = nullptr;
	if (layout != nullptr && layout->has_return_shape())
	{
		returnShape = layout->get_return_shape();
	}
	else
	{
		if (method != nullptr && method->get_return_type() != nullptr)
			returnShape = pragmatic.find_or_create_type_shape(model, method->get_return_type());

		if (layout != nullptr)
			layout->set_return_shape(returnShape);
	}

	const std::uint16_t argumentCount = layout != nullptr ? layout->get_argument_count() : 0;
	const std::uint16_t variableCount = layout != nullptr ? layout->get_variable_count() : 0;
	const std::uint16_t localCount = static_cast<std::uint16_t>(argumentCount + variableCount);

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

	auto local_type = [&](std::size_t index) -> StructSymbol*
	{
		if (layout == nullptr)
			return nullptr;

		return index < argumentCount
			? layout->get_argument_types()[index]
			: layout->get_variable_slots()[index - argumentCount].concreteType;
	};

	auto owns_local_data = [&](std::size_t index, const TypeShape* shape) -> bool
	{
		if (shape == nullptr)
			return false;

		if (index < argumentCount)
			return !shape->is_reference();

		const FrameSlotRecipe& recipe = layout->get_variable_slots()[index - argumentCount];
		if (recipe.verdict != PlacementVerdict::Stack)
			return false;

		return !shape->is_reference() || shape->get_size() > 0;
	};

	auto data_bytes = [&](const TypeShape* shape) -> std::size_t
	{
		if (shape == nullptr)
			return 0;

		std::size_t bytes = gmt::align_up(shape->get_size(), sizeof(void*));
		if (!shape->is_reference())
			return bytes;

		for (std::size_t i = 0; i < shape->get_slot_count(); ++i)
		{
			const TypeShape::SlotInfo& field = shape->get_slot(static_cast<std::uint32_t>(i));
			if (field.fieldShape != nullptr && !field.fieldShape->is_reference() && !field.isNullable)
				bytes += gmt::align_up(field.fieldShape->get_size(), sizeof(void*));
		}

		return bytes;
	};

	std::size_t localsBytes = 0;
	for (std::size_t i = 0; i < localCount; ++i)
	{
		const TypeShape* shape = local_shape(i);
		if (owns_local_data(i, shape))
			localsBytes += data_bytes(shape);
	}

	std::size_t evalSlotPayload = layout != nullptr ? layout->get_eval_slot_payload() : 0;
	if (layout == nullptr || !layout->is_complete())
		evalSlotPayload = (std::max)(evalSlotPayload, sizeof(void*));

	std::size_t evalStride = EntryHeaderSize + gmt::align_up((std::max)(evalSlotPayload, sizeof(void*)), sizeof(void*));
	std::size_t evalCapacity = 64;
	if (layout != nullptr && layout->is_complete())
		evalCapacity = (std::max)(static_cast<std::size_t>(layout->get_max_eval_depth()), static_cast<std::size_t>(8));

	const std::size_t objectBytes = gmt::align_up(sizeof(CallStackFrame), sizeof(void*));
	const std::size_t metadataBytes = static_cast<std::size_t>(localCount) * sizeof(CallStackFrame::LocalSlot);
	const std::size_t slotBytes = static_cast<std::size_t>(localCount) * sizeof(void*);
	const std::size_t totalBytes = objectBytes + metadataBytes + slotBytes + localsBytes + evalStride * evalCapacity;

	std::byte* block = pool != nullptr
		? pool->acquire(totalBytes)
		: static_cast<std::byte*>(gmt::mem_alloc(totalBytes));

	CallStackFrame* frame = new (block) CallStackFrame();
	frame->m_blockBytes = totalBytes;
	frame->m_layout = layout;
	frame->m_method = method;
	frame->m_previous = previous;
	frame->m_collector = collector;
	frame->m_argumentCount = argumentCount;
	frame->m_variableCount = variableCount;
	frame->m_localCount = localCount;
	frame->m_returnShape = returnShape;
	frame->m_returnResult = ObjectInstance(returnShape, nullptr, collector);

	frame->m_localSlots = reinterpret_cast<LocalSlot*>(block + objectBytes);
	frame->m_variableSlots = reinterpret_cast<std::byte**>(reinterpret_cast<std::byte*>(frame->m_localSlots) + metadataBytes);
	frame->m_localsRegion = reinterpret_cast<std::byte*>(frame->m_variableSlots) + slotBytes;
	frame->m_evalRegion = frame->m_localsRegion + localsBytes;
	frame->m_evalStride = evalStride;
	frame->m_evalCapacity = static_cast<std::uint32_t>(evalCapacity);

	std::memset(frame->m_variableSlots, 0, slotBytes);
	if (localsBytes > 0)
		std::memset(frame->m_localsRegion, 0, localsBytes);

	std::size_t initializationPins = 0;
	if (externalReturnDestination != nullptr)
	{
		frame->m_returnDestination = externalReturnDestination;
	}
	else if (returnShape != nullptr && returnShape->get_size() > 0)
	{
		if (collector == nullptr)
			throw std::runtime_error("a return destination requires a garbage collector");

		ObjectInstance destination = collector->allocate_instance(returnShape);
		collector->pin(destination);
		++initializationPins;
		frame->m_returnDestination = destination.get_memory();
	}

	std::size_t cursor = 0;
	for (std::size_t i = 0; i < localCount; ++i)
	{
		LocalSlot& local = frame->m_localSlots[i];
		local.shape = local_shape(i);
		local.dataAddress = nullptr;
		StructSymbol* type = local_type(i);
		local.isNullable = type != nullptr && type->get_kind() == SyntaxKind::NullableType;

		if (owns_local_data(i, local.shape))
		{
			local.dataAddress = frame->m_localsRegion + cursor;
			cursor += data_bytes(local.shape);

			if (!local.isNullable)
				frame->m_variableSlots[i] = local.dataAddress;

			if (local.shape->is_reference())
			{
				std::size_t fieldCursor = cursor - data_bytes(local.shape) + gmt::align_up(local.shape->get_size(), sizeof(void*));
				for (std::size_t j = 0; j < local.shape->get_slot_count(); ++j)
				{
					const TypeShape::SlotInfo& field = local.shape->get_slot(static_cast<std::uint32_t>(j));
					if (field.fieldShape == nullptr || field.fieldShape->is_reference() || field.isNullable)
						continue;

					std::byte* cell = frame->m_localsRegion + fieldCursor;
					fieldCursor += gmt::align_up(field.fieldShape->get_size(), sizeof(void*));
					std::memcpy(local.dataAddress + field.offset, &cell, sizeof(std::byte*));
				}
			}

			continue;
		}

		if (i >= argumentCount && local.shape != nullptr && !local.shape->is_reference() && !local.isNullable)
		{
			if (collector == nullptr)
				throw std::runtime_error("a heap local requires a garbage collector");

			ObjectInstance cell = collector->allocate_instance(local.shape);
			collector->pin(cell);
			++initializationPins;
			frame->m_variableSlots[i] = cell.get_memory();
		}
	}

	if (initializationPins > 0)
		collector->unpin(initializationPins);

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

MethodSymbol* CallStackFrame::get_method() const { return m_method; }
CallStackFrame* CallStackFrame::get_previous() const { return m_previous; }
std::size_t CallStackFrame::get_instruction_pointer() const { return m_instructionPointer; }
void CallStackFrame::set_instruction_pointer(std::size_t offset) { m_instructionPointer = offset; }
InterruptionReason CallStackFrame::get_interruption() const { return m_interruption; }
void CallStackFrame::set_interruption(InterruptionReason reason) { m_interruption = reason; }
ObjectInstance CallStackFrame::get_current_exception() const { return m_currentException; }
void CallStackFrame::set_current_exception(ObjectInstance value) { m_currentException = value; }
gmt::vector<std::size_t>& CallStackFrame::get_defer_stack() { return m_deferStack; }
gmt::vector<ExceptionHandlerRecord>& CallStackFrame::get_exception_handlers() { return m_exceptionHandlers; }
std::uint16_t CallStackFrame::get_argument_count() const { return m_argumentCount; }
std::uint16_t CallStackFrame::get_variable_count() const { return m_variableCount; }
std::uint32_t CallStackFrame::get_pending_children() const { return m_pendingChildren; }
void CallStackFrame::increment_pending_children() { ++m_pendingChildren; }
void CallStackFrame::decrement_pending_children() { if (m_pendingChildren > 0) --m_pendingChildren; }
std::byte* CallStackFrame::get_async_state() const { return m_asyncState; }
void CallStackFrame::set_async_state(std::byte* state) { m_asyncState = state; }
std::byte* CallStackFrame::get_awaited_state() const { return m_awaitedState; }
void CallStackFrame::set_awaited_state(std::byte* state) { m_awaitedState = state; }
bool CallStackFrame::get_suspended_at_return() const { return m_suspendedAtReturn; }
void CallStackFrame::set_suspended_at_return(bool value) { m_suspendedAtReturn = value; }
bool CallStackFrame::get_is_runnable() const { return m_runnable; }
void CallStackFrame::set_is_runnable(bool value) { m_runnable = value; }

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
	FrameEntryView view = entry_view(m_evalRegion + m_evalStride * m_evalDepth);
	if (view.isReferenceStorage)
	{
		std::byte* pointer;
		std::memcpy(&pointer, view.payloadAddress, sizeof(std::byte*));
		return ObjectInstance(view.shape, pointer, m_collector);
	}

	return ObjectInstance(view.shape, view.payloadAddress, m_collector);
}

ObjectInstance CallStackFrame::peek_entry() const
{
	if (m_evalDepth == 0)
		throw std::underflow_error("eval stack underflow");

	FrameEntryView view = entry_view(m_evalRegion + m_evalStride * (static_cast<std::size_t>(m_evalDepth) - 1));
	if (view.isReferenceStorage)
	{
		std::byte* pointer;
		std::memcpy(&pointer, view.payloadAddress, sizeof(std::byte*));
		return ObjectInstance(view.shape, pointer, m_collector);
	}

	return ObjectInstance(view.shape, view.payloadAddress, m_collector);
}

std::size_t CallStackFrame::get_eval_depth() const { return m_evalDepth; }
void CallStackFrame::reset_eval_depth(std::size_t depth) { m_evalDepth = static_cast<std::uint32_t>(depth); }

ObjectInstance CallStackFrame::peek_entry_at(std::size_t depthFromTop) const
{
	if (depthFromTop >= m_evalDepth)
		throw std::underflow_error("eval stack underflow");

	FrameEntryView view = entry_view(m_evalRegion + m_evalStride * (static_cast<std::size_t>(m_evalDepth) - 1 - depthFromTop));
	if (view.isReferenceStorage)
	{
		std::byte* pointer;
		std::memcpy(&pointer, view.payloadAddress, sizeof(std::byte*));
		return ObjectInstance(view.shape, pointer, m_collector);
	}

	return ObjectInstance(view.shape, view.payloadAddress, m_collector);
}

ObjectInstance CallStackFrame::get_local(std::uint16_t slot) const
{
	return ObjectInstance(m_localSlots[slot].shape, m_variableSlots[slot], m_collector);
}

void CallStackFrame::set_local(std::uint16_t slot, ObjectInstance value)
{
	if (slot >= m_localCount)
		throw std::runtime_error("local slot index is out of range");

	LocalSlot& local = m_localSlots[slot];
	if (value.get_shape() != nullptr && (local.shape == nullptr || local.shape->get_base_type()->get_kind() == SyntaxKind::TypeParameter))
		local.shape = value.get_shape();

	if (local.shape == nullptr || local.shape->is_reference())
	{
		m_variableSlots[slot] = value.get_memory();
		return;
	}

	if (value == nullptr)
	{
		m_variableSlots[slot] = nullptr;
		return;
	}

	std::byte* destination = local.dataAddress;
	if (destination == nullptr)
		destination = m_variableSlots[slot];

	if (destination == nullptr)
	{
		if (m_collector == nullptr)
			throw std::runtime_error("cannot allocate a local value cell without a garbage collector");

		ObjectInstance cell = m_collector->allocate_instance(local.shape);
		destination = cell.get_memory();
	}

	m_variableSlots[slot] = destination;
	std::memcpy(destination, value.get_memory(), local.shape->get_size());
	m_variableSlots[slot] = destination;
}

void CallStackFrame::push_local(std::uint16_t slot)
{
	const LocalSlot& local = m_localSlots[slot];
	std::byte* pointer = m_variableSlots[slot];
	if (pointer == nullptr || local.shape == nullptr || local.shape->is_reference())
	{
		push_entry(local.shape, true, &pointer, sizeof(std::byte*));
		return;
	}

	push_entry(local.shape, false, pointer, local.shape->get_size());
}

ObjectInstance CallStackFrame::get_local_address(std::uint16_t slot) const
{
	return get_local(slot);
}

ObjectInstance CallStackFrame::ensure_local_storage(std::uint16_t slot)
{
	LocalSlot& local = m_localSlots[slot];
	if (local.dataAddress != nullptr)
		return ObjectInstance(local.shape, local.dataAddress, m_collector);

	if (m_variableSlots[slot] != nullptr)
		return get_local(slot);

	if (m_collector == nullptr || local.shape == nullptr)
		return ObjectInstance::null();

	ObjectInstance instance = m_collector->allocate_instance(local.shape);
	m_variableSlots[slot] = instance.get_memory();
	return instance;
}

FrameEntryView CallStackFrame::get_local_view(std::uint16_t slot) const
{
	FrameEntryView view{};
	view.shape = m_localSlots[slot].shape;
	view.isReferenceStorage = true;
	view.payloadAddress = reinterpret_cast<std::byte*>(m_variableSlots + slot);
	return view;
}

std::byte* CallStackFrame::get_local_entry(std::uint16_t slot) const
{
	return reinterpret_cast<std::byte*>(m_variableSlots + slot);
}

FrameEntryView CallStackFrame::get_return_view() const
{
	FrameEntryView view{};
	view.shape = m_returnShape;
	view.isReferenceStorage = true;
	view.payloadAddress = reinterpret_cast<std::byte*>(const_cast<std::byte**>(&m_returnDestination));
	return view;
}

void CallStackFrame::complete_return(ObjectInstance value)
{
	if (value == nullptr)
	{
		m_returnResult = ObjectInstance(m_returnShape, nullptr, m_collector);
		return;
	}

	if (m_returnShape != nullptr && !m_returnShape->is_reference())
	{
		if (m_returnDestination != nullptr && value.get_memory() != m_returnDestination)
			std::memcpy(m_returnDestination, value.get_memory(), m_returnShape->get_size());

		m_returnResult = ObjectInstance(m_returnShape, m_returnDestination, m_collector);
		return;
	}

	m_returnResult = ObjectInstance(m_returnShape, value.get_memory(), m_collector);
}

void CallStackFrame::set_return_result(ObjectInstance value)
{
	m_returnResult = ObjectInstance(m_returnShape, value.get_memory(), m_collector);
}

ObjectInstance CallStackFrame::get_return_slot() const { return m_returnResult; }
ObjectInstance CallStackFrame::get_return_destination() const { return ObjectInstance(m_returnShape, m_returnDestination, m_collector); }
std::byte* CallStackFrame::get_return_entry() const { return m_returnDestination; }
std::size_t CallStackFrame::get_return_payload_bytes() const { return m_returnShape != nullptr ? m_returnShape->get_size() : 0; }
bool CallStackFrame::has_external_return_slot() const { return true; }
