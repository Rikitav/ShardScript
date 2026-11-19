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

std::byte* CallStackFrame::allocate_local_cell(std::uint16_t slot)
{
	if (m_collector == nullptr)
		throw std::runtime_error("cannot allocate a local value cell without a garbage collector");

	ObjectInstance cell = m_collector->allocate_instance(m_localSlots[slot].shape);
	return cell.get_memory();
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
		{
			// an open return type has no shape; the returned value carries its own
			if (!model.contains_type_parameter(method->get_return_type()))
				returnShape = pragmatic.find_or_create_type_shape(model, method->get_return_type());
		}

		if (layout != nullptr)
			layout->set_return_shape(returnShape);
	}

	const std::uint16_t argumentCount = layout != nullptr ? layout->get_argument_count() : 0;
	const std::uint16_t variableCount = layout != nullptr ? layout->get_variable_count() : 0;
	const std::uint16_t localCount = static_cast<std::uint16_t>(argumentCount + variableCount);

	// a sealed layout carries its resolved slots; the runtime then only copies
	// the plan instead of re-resolving every slot through the shape cache
	const std::span<const FrameSlotPlan> plans = layout != nullptr
		? layout->ensure_slot_plans()
		: std::span<const FrameSlotPlan>();

	const bool planned = layout != nullptr
		&& layout->slot_plans_complete()
		&& plans.size() == static_cast<std::size_t>(localCount);

	auto local_shape = [&](std::size_t index) -> const TypeShape*
	{
		if (planned)
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

		// an open type has no layout of its own: the value stored in the slot
		// carries its own concrete shape, so the slot needs no shape here
		if (model.contains_type_parameter(type))
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
	if (planned)
	{
		localsBytes = layout->planned_locals_bytes();
	}
	else
	{
		for (std::size_t i = 0; i < localCount; ++i)
		{
			const TypeShape* shape = local_shape(i);
			if (owns_local_data(i, shape))
				localsBytes += data_bytes(shape);
		}
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


