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
#include <stdexcept>

using namespace shard;

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

bool GarbageCollector::is_heap_pointer(std::byte* pointer)
{
	if (pointer == nullptr)
		return false;

	GcHeader* header = get_header(pointer);
	return header->magic == Magic;
}

bool GarbageCollector::is_heap_backed(std::byte* payload) const
{
	return payload != nullptr && m_heap.contains(payload);
}

ObjectInstance GarbageCollector::allocate_block(const TypeShape* shape, std::size_t payloadBytes)
{
	if (m_allocatedSinceCollection >= m_collectionThreshold)
		collect();

	GcHeader* header = static_cast<GcHeader*>(gmt::mem_alloc(sizeof(GcHeader) + payloadBytes));
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

	return ObjectInstance(shape, payload);
}

ObjectInstance GarbageCollector::allocate_instance(const TypeShape* shape)
{
	return allocate_block(shape, shape->get_size());
}

ObjectInstance GarbageCollector::allocate_instance(const TypeShape* shape, std::size_t payloadBytes)
{
	return allocate_block(shape, payloadBytes);
}

ObjectInstance GarbageCollector::allocate_array(const TypeShape* arrayShape, const TypeShape* elementShape, std::int64_t length)
{
	if (length < 0)
		throw std::runtime_error("negative array length");

	std::size_t elementStride = elementShape->is_reference() ? sizeof(void*) : elementShape->get_size();
	std::size_t payloadBytes = sizeof(std::int64_t) + elementStride * static_cast<std::size_t>(length);

	ObjectInstance instance = allocate_block(arrayShape, payloadBytes);
	instance.write_integer(length);
	return instance;
}

ObjectInstance GarbageCollector::allocate_string(std::wstring_view value)
{
	const TypeShape* stringShape = m_pragmatic.find_or_create_type_shape(m_model, m_model.get_table().get_string_primitive());

	std::size_t payloadBytes = sizeof(std::int64_t) + (value.size() + 1) * sizeof(wchar_t);
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
		return ObjectInstance(existing->second.shape, existing->second.storage);

	const TypeShape* shape = m_pragmatic.find_or_create_type_shape(m_model, field->get_return_type());
	std::size_t bytes = shape->is_reference() ? sizeof(void*) : shape->get_size();

	StaticStorage storage{};
	storage.shape = shape;
	storage.storage = static_cast<std::byte*>(gmt::mem_alloc(bytes));
	m_statics.emplace(key, storage);

	return ObjectInstance(shape, storage.storage);
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
		ArrayTypeSymbol* arrayType = static_cast<ArrayTypeSymbol*>(baseType);
		const TypeShape* elementShape = m_pragmatic.find_or_create_type_shape(m_model, arrayType->get_underlaying_type());
		if (!elementShape->is_reference())
			return;

		std::int64_t length;
		std::memcpy(&length, payload, sizeof(std::int64_t));

		std::byte* elements = payload + sizeof(std::int64_t);
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
		if (slot.fieldShape == nullptr || !slot.fieldShape->is_reference())
			continue;

		std::byte* fieldPointer;
		std::memcpy(&fieldPointer, payload + slot.offset, sizeof(std::byte*));
		trace_pointer(fieldPointer);
	}
}

void GarbageCollector::trace_frame(CallStackFrame* frame)
{
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

		if (view.shape != nullptr && view.shape->is_reference())
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

		if (storage.shape->is_reference())
		{
			std::byte* pointer;
			std::memcpy(&pointer, storage.storage, sizeof(std::byte*));
			trace_pointer(pointer);
			continue;
		}

		trace_object_fields(storage.shape, storage.storage);
	}

	for (const auto& [key, instance] : m_internedStrings)
		trace_pointer(instance.get_memory());

	for (const ObjectInstance& pinned : m_pinned)
	{
		if (pinned.get_shape() != nullptr && pinned.get_shape()->is_reference())
			trace_pointer(pinned.get_memory());
	}

	for (const ObjectInstance& root : m_roots)
	{
		if (root.get_shape() != nullptr && root.get_shape()->is_reference())
			trace_pointer(root.get_memory());
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
