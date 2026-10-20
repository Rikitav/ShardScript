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

#include <cstring>

#include <mimalloc.h>

#include <algorithm>
#include <cstring>
#include <cstdlib>
#include <stdexcept>

#ifdef SHARD_GC_NO_MIMALLOC
	#define gc_malloc ::malloc
	#define gc_free ::free
#else
	#define gc_malloc mi_zalloc
	#define gc_free mi_free
#endif

using namespace shard;

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

	GcHeader* header = static_cast<GcHeader*>(gc_malloc(sizeof(GcHeader) + payloadBytes));
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
	m_internedStrings.emplace(std::wstring(value), instance);
	return instance;
}

ObjectInstance GarbageCollector::get_static_storage(gmt::Ref<FieldSymbol> field)
{
	const std::size_t key = field.get_offset();
	auto existing = m_statics.find(key);
	if (existing != m_statics.end())
		return ObjectInstance(existing->second.shape, existing->second.storage);

	const TypeShape* shape = m_pragmatic.find_or_create_type_shape(m_model, field->get_return_type());
	std::size_t bytes = shape->is_reference() ? sizeof(void*) : shape->get_size();

	StaticStorage storage;
	storage.shape = shape;
	storage.storage = static_cast<std::byte*>(gc_malloc(bytes));
	m_statics.emplace(key, storage);

	return ObjectInstance(shape, storage.storage);
}

void GarbageCollector::register_frame(CallStackFrame* frame)
{
	m_frames.push_back(frame);
}

void GarbageCollector::unregister_frame(CallStackFrame* frame)
{
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
	trace_object_fields(header->shape, pointer);
}

void GarbageCollector::trace_object_fields(const TypeShape* shape, std::byte* payload)
{
	if (shape == nullptr)
		return;

	gmt::Ref<StructSymbol> baseType = shape->get_base_type();
	if (baseType.is_null())
		return;

	SyntaxKind kind = baseType->get_kind();
	if (baseType == m_model.get_table().get_string_primitive())
		return;

	if (kind == SyntaxKind::ArrayType)
	{
		gmt::Ref<ArrayTypeSymbol> arrayType = baseType.as<ArrayTypeSymbol>();
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
	for (auto choice = m_heap.begin(); choice != m_heap.end();)
	{
		GcHeader* header = get_header(*choice);
		if (header->marked == 0)
		{
			gc_free(header);
			choice = m_heap.erase(choice);
			continue;
		}

		header->marked = 0;
		liveBytes += sizeof(GcHeader) + header->payloadBytes;
		++choice;
	}

	m_allocatedSinceCollection = liveBytes;
	m_collectionThreshold = (std::max)(m_collectionThreshold, liveBytes * 2);
}

void GarbageCollector::collect()
{
	++m_totalCollections;

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

	sweep();
}

void GarbageCollector::terminate()
{
	for (std::byte* payload : m_heap)
		gc_free(get_header(payload));

	m_heap.clear();

	for (auto& [key, storage] : m_statics)
		gc_free(storage.storage);

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
