#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/semantic/symbols/TypeSymbol.hpp>
#include <shard/semantic/symbols/FieldSymbol.hpp>
#include <shard/runtime/TypeShape.hpp>

#include <string>
#include <cstdint>
#include <span>
#include <memory>

namespace shard
{
	class CallStackFrame;
	class MethodSymbol;
	class ObjectInstance;
	class GarbageCollector;

	/// <summary>
	/// Lightweight value wrapper over a single byte pointer plus the type
	/// metadata needed to interpret it. It is NOT an owning object and is not
	/// embedded inside heap allocations.
	///
	/// The pointer this wrapper holds is either:
	///   - the payload of a GC-managed heap block whose layout is
	///     `[GcHeader][payload]` (by-reference objects), or
	///   - inline bytes inside a stack frame or an enclosing object's payload
	///     (by-value objects).
	///
	/// The GcHeader is recovered by walking back `sizeof(GcHeader)` bytes from
	/// the payload pointer and validating the magic; a null by-reference value
	/// has a null payload pointer and therefore no GcHeader.
	/// </summary>
	class SHARD_API ObjectInstance
	{
		friend class GarbageCollector;

	public:
		struct GcHeader
		{
			static constexpr std::uint64_t MAGIC = 0x5348415244474348ULL; // "SHARDGCH"
			std::uint64_t Magic;
			std::int64_t ReferencesCounter;
			TypeShape* Shape;
			bool Terminated;
		};

	private:
		const TypeSymbol* m_info;
		TypeShape* m_shape;
		std::byte* m_rawMemoryPtr;

		[[nodiscard]] inline GcHeader* getGcHeader() const
		{
			if (m_rawMemoryPtr == nullptr)
				return nullptr;

			GcHeader* header = reinterpret_cast<GcHeader*>(m_rawMemoryPtr - sizeof(GcHeader));
			return header->Magic == GcHeader::MAGIC ? header : nullptr;
		}

		[[nodiscard]] inline bool isHeapBacked() const
		{
			return getGcHeader() != nullptr;
		}

	public:
		inline ObjectInstance()
			: m_info(nullptr), m_shape(nullptr), m_rawMemoryPtr(nullptr) { }

		inline ObjectInstance(const TypeSymbol* info, TypeShape* shape, std::byte* memory)
			: m_info(info), m_shape(shape), m_rawMemoryPtr(memory) { }

		inline ObjectInstance(TypeShape* shape, std::byte* memory)
			: m_info(shape != nullptr ? shape->BaseType : nullptr), m_shape(shape), m_rawMemoryPtr(memory) { }

		~ObjectInstance() = default;

		// Identity is the payload pointer (by-reference instances share a heap
		// payload; null equals null).
		inline bool operator==(const ObjectInstance& other) const { return m_rawMemoryPtr == other.m_rawMemoryPtr; }
		inline bool operator!=(const ObjectInstance& other) const { return m_rawMemoryPtr != other.m_rawMemoryPtr; }

		[[nodiscard]] const TypeSymbol* getInfo() const;
		[[nodiscard]] TypeShape* getShape() const;
		[[nodiscard]] std::byte* getMemory() const;
		[[nodiscard]] std::int64_t getReferencesCounter() const;

		// Fields. A by-reference field slot stores a raw payload pointer; a
		// by-value field slot stores the value's bytes inline.
		ObjectInstance GetField(std::uint32_t slot);
		ObjectInstance GetField(const FieldSymbol* field);
		void SetField(std::uint32_t slot, ObjectInstance instance);
		void SetField(const FieldSymbol* field, ObjectInstance instance);

		// Arrays — same contract as GetField.
		std::size_t GetArrayLength() const;
		ObjectInstance GetElement(std::size_t index, CallStackFrame* frame = nullptr);
		void SetElement(std::size_t index, ObjectInstance instance, CallStackFrame* frame = nullptr);
		bool IsInBounds(std::size_t index);

		void IncrementReference() const;
		void DecrementReference() const;

		// Null exists only for by-reference values: a null payload pointer.
		// By-value values always have a valid inline payload and are never null.
		[[nodiscard]] bool IsNullInstance() const;

		void WriteBoolean(const bool& value) const;
		void WriteInteger(const std::int64_t& value) const;
		void WriteDouble(const double& value) const;
		void WriteCharacter(const wchar_t& value) const;
		void WriteByte(const std::uint8_t& value) const;

		bool& AsBoolean() const;
		std::int64_t& AsInteger() const;
		double& AsDouble() const;
		wchar_t& AsCharacter() const;
		std::uint8_t& AsByte() const;
		const wchar_t* AsString() const;
		std::int64_t& AsStringLength() const;
		void* AsNint() const;

		/// <summary>
		/// Helper method to unwrap native object handle, saved as nint
		/// </summary>
		/// <typeparam name="T"></typeparam>
		/// <returns></returns>
		template<typename T>
		T* AsNint() const
		{
			return static_cast<T*>(AsNint());
		}

		std::byte* OffsetMemory(const std::size_t offset, const std::size_t size) const;
		void ReadMemory(const std::size_t offset, const std::size_t size, void* dst) const;
		void WriteMemory(const std::size_t offset, const std::size_t size, const void* src) const;
	};

	/// <summary>
	/// RAII wrapper that keeps an ObjectInstance alive across an async boundary.
	/// Holds the wrapper by value and roots/unroots the underlying heap object
	/// (if any) via its GcHeader.
	/// </summary>
	struct SHARD_API ObjectRef
	{
		ObjectInstance Value;

		ObjectRef() = default;

		explicit ObjectRef(ObjectInstance value) : Value(value)
		{
			if (!Value.IsNullInstance())
				Value.IncrementReference();
		}

		explicit ObjectRef(ObjectInstance* value)
			: Value(value != nullptr ? *value : ObjectInstance())
		{
			if (!Value.IsNullInstance())
				Value.IncrementReference();
		}

		~ObjectRef()
		{
			if (!Value.IsNullInstance())
				Value.DecrementReference();
		}

		ObjectRef(const ObjectRef&) = delete;
		ObjectRef& operator=(const ObjectRef&) = delete;

		ObjectRef(ObjectRef&& other) noexcept : Value(other.Value)
		{
			other.Value = ObjectInstance();
		}

		ObjectRef& operator=(ObjectRef&& other) noexcept
		{
			if (this != &other)
			{
				if (!Value.IsNullInstance())
					Value.DecrementReference();
				Value = other.Value;
				other.Value = ObjectInstance();
			}
			return *this;
		}

		[[nodiscard]] bool IsNull() const { return Value.IsNullInstance(); }
	};
}
