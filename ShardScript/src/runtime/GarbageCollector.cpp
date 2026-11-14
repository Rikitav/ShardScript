#include <shard/runtime/GarbageCollector.hpp>

#include <shard/runtime/CallStackFrame.hpp>

#include <shard/parsing/SyntaxKind.hpp>

#include <shard/pragmatic/PragmaticModel.hpp>
#include <shard/pragmatic/TypeShape.hpp>

#include <shard/semantic/SemanticModel.hpp>
#include <shard/semantic/SymbolTable.hpp>
#include <shard/semantic/symbols/ArrayTypeSymbol.hpp>
#include <shard/semantic/symbols/FieldSymbol.hpp>
#include <shard/semantic/symbols/StructSymbol.hpp>

#include <gmt/Allocator.hpp>
#include <gmt/Containers.hpp>

#include <algorithm>
#include <cstdlib>
#include <limits>
#include <stdexcept>
#include <mutex>

using namespace shard;

namespace
{
	std::size_t checked_add(std::size_t left, std::size_t right)
	{
		if (left > (std::numeric_limits<std::size_t>::max)() - right)
			throw std::length_error("allocation size overflow");

		return left + right;
	}

	std::size_t checked_multiply(std::size_t left, std::size_t right)
	{
		if (right != 0 && left > (std::numeric_limits<std::size_t>::max)() / right)
			throw std::length_error("allocation size overflow");

		return left * right;
	}
}

std::byte* const GarbageCollector::HeapTable::Tombstone = reinterpret_cast<std::byte*>(static_cast<std::uintptr_t>(1));

GarbageCollector::HeapTable::HeapTable()
	: m_slots(64, nullptr)
{ }

std::size_t GarbageCollector::HeapTable::home(std::byte* value) const
{
	const std::uintptr_t raw = reinterpret_cast<std::uintptr_t>(value) >> 3;
	const std::uint64_t mixed = static_cast<std::uint64_t>(raw) * 0x9E3779B97F4A7C15ull;
	return static_cast<std::size_t>(mixed) & (m_slots.size() - 1);
}

std::size_t GarbageCollector::HeapTable::locate(std::byte* value) const
{
	const std::size_t mask = m_slots.size() - 1;
	std::size_t slot = home(value);

	while (true)
	{
		std::byte* current = m_slots[slot];
		if (current == value)
			return slot;

		if (current == nullptr)
			return m_slots.size();

		slot = (slot + 1) & mask;
	}
}

void GarbageCollector::HeapTable::rehash(std::size_t capacity)
{
	gmt::vector<std::byte*> fresh(capacity, nullptr);
	const std::size_t mask = capacity - 1;

	for (std::byte* slot : m_slots)
	{
		if (slot == nullptr || slot == Tombstone)
			continue;

		const std::uintptr_t raw = reinterpret_cast<std::uintptr_t>(slot) >> 3;
		std::size_t index = static_cast<std::size_t>(static_cast<std::uint64_t>(raw) * 0x9E3779B97F4A7C15ull) & mask;
		while (fresh[index] != nullptr)
			index = (index + 1) & mask;

		fresh[index] = slot;
	}

	m_slots = std::move(fresh);
	m_tombstones = 0;
}

void GarbageCollector::HeapTable::insert(std::byte* value)
{
	if ((m_size + m_tombstones + 1) * 10 >= m_slots.size() * 7)
	{
		// grow only when live objects alone need the room; otherwise a rehash
		// at the same capacity clears the tombstones left by a sweep
		const std::size_t capacity = (m_size + 1) * 10 >= m_slots.size() * 7
			? m_slots.size() * 2
			: m_slots.size();
		rehash(capacity);
	}

	const std::size_t mask = m_slots.size() - 1;
	std::size_t slot = home(value);
	std::size_t tombstone = m_slots.size();

	while (true)
	{
		std::byte* current = m_slots[slot];
		if (current == value)
			return;

		if (current == nullptr)
			break;

		if (current == Tombstone && tombstone == m_slots.size())
			tombstone = slot;

		slot = (slot + 1) & mask;
	}

	if (tombstone != m_slots.size())
		slot = tombstone;

	m_slots[slot] = value;
	++m_size;
}

bool GarbageCollector::HeapTable::erase(std::byte* value)
{
	const std::size_t slot = locate(value);
	if (slot == m_slots.size())
		return false;

	m_slots[slot] = Tombstone;
	--m_size;
	++m_tombstones;
	return true;
}

bool GarbageCollector::HeapTable::contains(std::byte* value) const
{
	if (value == nullptr || value == Tombstone)
		return false;

	return locate(value) != m_slots.size();
}

void GarbageCollector::HeapTable::clear()
{
	for (std::byte*& slot : m_slots)
		slot = nullptr;

	m_size = 0;
	m_tombstones = 0;
}

std::size_t GarbageCollector::HeapTable::size() const
{
	return m_size;
}

GarbageCollector::GarbageCollector(SemanticModel& model, PragmaticModel& pragmatic)
	: m_model(model)
	, m_pragmatic(pragmatic)
{ }

GarbageCollector::~GarbageCollector()
{
	terminate();
}

GarbageCollector::GcHeader* GarbageCollector::get_header(std::byte* payload)
{
	return reinterpret_cast<GcHeader*>(payload - sizeof(GcHeader));
}

bool GarbageCollector::is_heap_backed(std::byte* payload) const
{
	return payload != nullptr && m_heap.contains(payload);
}

ObjectInstance GarbageCollector::allocate_block(const TypeShape* shape, std::size_t payloadBytes)
{
	if (m_allocatedSinceCollection >= m_collectionThreshold)
		collect();

	const std::size_t allocationBytes = checked_add(sizeof(GcHeader), payloadBytes);
	GcHeader* header = static_cast<GcHeader*>(gmt::mem_alloc(allocationBytes));
	if (header == nullptr)
		throw std::bad_alloc();

	header->magic = Magic;
	header->shape = shape;
	header->marked = 0;
	header->payloadBytes = payloadBytes;

	std::byte* payload = reinterpret_cast<std::byte*>(header + 1);
	m_heap.insert(payload);

	// the instance is GC-visible from here on: zero the payload so marking
	// never chases garbage pointers from not-yet-assigned reference slots
	std::memset(payload, 0, payloadBytes);

	m_allocatedSinceCollection += sizeof(GcHeader) + payloadBytes;

	++m_totalAllocations;
	m_totalAllocatedBytes += sizeof(GcHeader) + payloadBytes;

	return ObjectInstance(shape, payload, this);
}

ObjectInstance GarbageCollector::allocate_instance(const TypeShape* shape)
{
	ObjectInstance instance = allocate_block(shape, shape->get_size());
	pin(instance);

	for (std::size_t i = 0; i < shape->get_slot_count(); ++i)
	{
		const TypeShape::SlotInfo& slot = shape->get_slot(static_cast<std::uint32_t>(i));
		if (slot.fieldShape == nullptr || slot.fieldShape->is_reference() || slot.isNullable)
			continue;

		ObjectInstance cell = allocate_instance(slot.fieldShape);
		std::byte* pointer = cell.get_memory();
		std::memcpy(instance.get_memory() + slot.offset, &pointer, sizeof(std::byte*));
	}

	unpin(1);
	return instance;
}

ObjectInstance GarbageCollector::allocate_instance(const TypeShape* shape, std::size_t payloadBytes)
{
	return allocate_block(shape, payloadBytes);
}

ObjectInstance GarbageCollector::allocate_array(const TypeShape* arrayShape, const TypeShape* elementShape, std::int64_t length)
{
	if (length < 0)
		throw std::runtime_error("negative array length");

	const std::size_t elementsBytes = checked_multiply(sizeof(void*), static_cast<std::size_t>(length));
	const std::size_t payloadBytes = checked_add(sizeof(std::int64_t), elementsBytes);

	ObjectInstance instance = allocate_block(arrayShape, payloadBytes);
	instance.write_integer(length);

	ArrayTypeSymbol* arrayType = static_cast<ArrayTypeSymbol*>(arrayShape->get_base_type());
	bool nullableElements = arrayType->get_underlaying_type()->get_kind() == SyntaxKind::NullableType;
	if (!elementShape->is_reference() && !nullableElements && length > 0)
	{
		pin(instance);
		const std::size_t valueBytes = checked_multiply(elementShape->get_size(), static_cast<std::size_t>(length));
		ObjectInstance cells = allocate_instance(elementShape, valueBytes);
		std::byte* elements = instance.get_memory() + sizeof(std::int64_t);
		for (std::int64_t i = 0; i < length; ++i)
		{
			std::byte* pointer = cells.get_memory() + i * elementShape->get_size();
			std::memcpy(elements + i * sizeof(std::byte*), &pointer, sizeof(std::byte*));
		}
		unpin(1);
	}

	return instance;
}

ObjectInstance GarbageCollector::allocate_string(std::wstring_view value)
{
	if (value.size() > static_cast<std::size_t>((std::numeric_limits<std::int64_t>::max)()))
		throw std::length_error("string is too long");

	const TypeShape* stringShape = m_pragmatic.find_or_create_type_shape(m_model, m_model.get_table().get_string_primitive());

	const std::size_t characterCount = checked_add(value.size(), 1);
	const std::size_t characterBytes = checked_multiply(characterCount, sizeof(wchar_t));
	const std::size_t payloadBytes = checked_add(sizeof(std::int64_t), characterBytes);
	ObjectInstance instance = allocate_block(stringShape, payloadBytes);

	instance.write_integer(static_cast<std::int64_t>(value.size()));
	std::memcpy(instance.get_memory() + sizeof(std::int64_t), value.data(), value.size() * sizeof(wchar_t));

	return instance;
}

ObjectInstance GarbageCollector::intern_string(std::wstring_view value)
{
	auto existing = m_internedStrings.find(value);
	if (existing != m_internedStrings.end())
		return existing->second;

	ObjectInstance instance = allocate_string(value);
	m_internedStrings.emplace(gmt::wstring(value), instance);
	return instance;
}

ObjectInstance GarbageCollector::get_static_storage(FieldSymbol* field)
{
	const std::size_t key = reinterpret_cast<std::size_t>(field);
	auto existing = m_statics.find(key);
	if (existing != m_statics.end())
		return ObjectInstance(existing->second.shape, existing->second.storage, this);

	const TypeShape* shape = m_pragmatic.find_or_create_type_shape(m_model, field->get_return_type());

	StaticStorage storage{};
	storage.shape = shape;
	storage.storage = static_cast<std::byte*>(gmt::mem_alloc(sizeof(void*)));
	std::memset(storage.storage, 0, sizeof(void*));
	m_statics.emplace(key, storage);

	if (!shape->is_reference() && field->get_return_type()->get_kind() != SyntaxKind::NullableType)
	{
		ObjectInstance cell = allocate_instance(shape);
		std::byte* pointer = cell.get_memory();
		std::memcpy(storage.storage, &pointer, sizeof(std::byte*));
	}

	return ObjectInstance(shape, storage.storage, this);
}

void GarbageCollector::register_frame(CallStackFrame* frame)
{
	m_frames.push_back(frame);
}

void GarbageCollector::unregister_frame(CallStackFrame* frame)
{
	// frames unwind strictly LIFO, so the common case is a tail pop; keep
	// the search as a fallback for any non-stack-disciplined teardown
	if (!m_frames.empty() && m_frames.back() == frame)
	{
		m_frames.pop_back();
		return;
	}

	auto choice = std::find(m_frames.begin(), m_frames.end(), frame);
	if (choice != m_frames.end())
		m_frames.erase(choice);
}

void GarbageCollector::pin(ObjectInstance instance)
{
	m_pinned.push_back(instance);
}

void GarbageCollector::unpin(std::size_t count)
{
	if (count > m_pinned.size())
	{
		m_pinned.clear();
		return;
	}

	m_pinned.resize(m_pinned.size() - count);
}

void GarbageCollector::add_root(ObjectInstance instance)
{
	if (instance == nullptr)
		return;

	for (const ObjectInstance& existing : m_roots)
	{
		if (existing.get_memory() == instance.get_memory())
			return;
	}

	m_roots.push_back(instance);
}

void GarbageCollector::remove_root(ObjectInstance instance)
{
	if (instance == nullptr)
		return;

	for (std::size_t i = 0; i < m_roots.size(); ++i)
	{
		if (m_roots[i].get_memory() != instance.get_memory())
			continue;

		m_roots[i] = m_roots.back();
		m_roots.pop_back();
		return;
	}
}

void GarbageCollector::trace_pointer(std::byte* pointer)
{
	if (pointer == nullptr)
		return;

	if (!is_heap_backed(pointer))
		return;

	GcHeader* header = get_header(pointer);
	if (header->marked != 0)
		return;

	header->marked = 1;
	m_markStack.push_back(pointer);
}

void GarbageCollector::drain_mark_stack()
{
	while (!m_markStack.empty())
	{
		std::byte* payload = m_markStack.back();
		m_markStack.pop_back();
		trace_object_fields(get_header(payload)->shape, payload);
	}
}

void GarbageCollector::trace_object_fields(const TypeShape* shape, std::byte* payload)
{
	if (shape == nullptr)
		return;

	StructSymbol* baseType = shape->get_base_type();
	if (baseType == nullptr)
		return;

	SyntaxKind kind = baseType->get_kind();
	if (baseType == m_model.get_table().get_string_primitive())
		return;

	if (kind == SyntaxKind::ArrayType)
	{
		std::int64_t length;
		std::memcpy(&length, payload, sizeof(std::int64_t));

		ArrayTypeSymbol* arrayType = static_cast<ArrayTypeSymbol*>(baseType);
		const TypeShape* elementShape = shape->get_element_shape();
		std::byte* elements = payload + sizeof(std::int64_t);
		if (!elementShape->is_reference() && arrayType->get_underlaying_type()->get_kind() != SyntaxKind::NullableType)
		{
			if (length > 0)
			{
				std::byte* block;
				std::memcpy(&block, elements, sizeof(std::byte*));
				trace_pointer(block);
			}
			return;
		}

		for (std::int64_t i = 0; i < length; ++i)
		{
			std::byte* element;
			std::memcpy(&element, elements + i * sizeof(std::byte*), sizeof(std::byte*));
			trace_pointer(element);
		}

		return;
	}

	for (std::size_t i = 0; i < shape->get_slot_count(); ++i)
	{
		const TypeShape::SlotInfo& slot = shape->get_slot(static_cast<std::uint32_t>(i));
		std::byte* fieldPointer;
		std::memcpy(&fieldPointer, payload + slot.offset, sizeof(std::byte*));
		trace_pointer(fieldPointer);
	}
}

void GarbageCollector::trace_frame(CallStackFrame* frame)
{
	auto traceValue = [this](ObjectInstance value)
	{
		if (value == nullptr || value.get_shape() == nullptr)
			return;

		if (is_heap_backed(value.get_memory()))
			trace_pointer(value.get_memory());
		else
			trace_object_fields(value.get_shape(), value.get_memory());
	};

	frame->for_each_local([&traceValue](ObjectInstance value) { traceValue(value); });
	traceValue(frame->get_return_destination());
	traceValue(frame->get_return_slot());
	traceValue(frame->get_current_exception());

	frame->for_each_live_entry([this](const FrameEntryView& view)
	{
		if (view.isReferenceStorage)
		{
			std::byte* pointer;
			std::memcpy(&pointer, view.payloadAddress, sizeof(std::byte*));

			if (pointer == nullptr)
				return;

			if (is_heap_backed(pointer))
			{
				trace_pointer(pointer);
				return;
			}

			trace_object_fields(view.shape, pointer);
			return;
		}

		if (view.shape != nullptr)
			trace_object_fields(view.shape, view.payloadAddress);
	});
}

void GarbageCollector::sweep()
{
	std::size_t liveBytes = 0;
	m_heap.sweep([&](std::byte* payload)
	{
		GcHeader* header = get_header(payload);
		if (header->marked == 0)
		{
			gmt::mem_free(header);
			return true;
		}

		header->marked = 0;
		liveBytes += sizeof(GcHeader) + header->payloadBytes;
		return false;
	});

	m_allocatedSinceCollection = liveBytes;
	m_collectionThreshold = (std::max)(m_collectionThreshold, liveBytes * 2);
}

void GarbageCollector::collect()
{
	++m_totalCollections;
	m_markStack.clear();

	for (CallStackFrame* frame : m_frames)
		trace_frame(frame);

	for (const auto& [key, storage] : m_statics)
	{
		if (storage.shape == nullptr)
			continue;

		std::byte* pointer;
		std::memcpy(&pointer, storage.storage, sizeof(std::byte*));
		trace_pointer(pointer);
	}

	for (const auto& [key, instance] : m_internedStrings)
		trace_pointer(instance.get_memory());

	for (const ObjectInstance& pinned : m_pinned)
	{
		const TypeShape* shape = pinned.get_shape();
		if (shape == nullptr)
			continue;

		if (is_heap_backed(pinned.get_memory()))
			trace_pointer(pinned.get_memory());
		else
			trace_object_fields(shape, pinned.get_memory());
	}

	for (const ObjectInstance& root : m_roots)
	{
		const TypeShape* shape = root.get_shape();
		if (shape == nullptr)
			continue;

		if (is_heap_backed(root.get_memory()))
			trace_pointer(root.get_memory());
		else
			trace_object_fields(shape, root.get_memory());
	}

	// roots only enqueue their heap children; drain iteratively instead of
	// recursing through trace_pointer -> trace_object_fields
	drain_mark_stack();

	sweep();
}

void GarbageCollector::terminate()
{
	m_heap.for_each([](std::byte* payload)
	{
		gmt::mem_free(get_header(payload));
	});
	m_heap.clear();

	for (auto& [key, storage] : m_statics)
		gmt::mem_free(storage.storage);

	m_statics.clear();
	m_internedStrings.clear();
}

std::size_t GarbageCollector::get_instance_count() const
{
	return m_heap.size();
}

std::size_t GarbageCollector::get_total_allocations() const
{
	return m_totalAllocations;
}

std::size_t GarbageCollector::get_total_allocated_bytes() const
{
	return m_totalAllocatedBytes;
}

std::size_t GarbageCollector::get_total_collections() const
{
	return m_totalCollections;
}
