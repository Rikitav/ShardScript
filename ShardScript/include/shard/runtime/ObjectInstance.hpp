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

	class SHARD_API ObjectInstance
	{
	public:
		struct GcHeader
		{
			static constexpr std::uint64_t MAGIC = 0x5348415244474348ULL; // "SHARDGCH"
			std::uint64_t Magic;
			std::int64_t ReferencesCounter;
			bool Terminated;
		};

	private:
		const TypeSymbol* m_info;
		TypeShape* m_shape;
		void* m_rawMemoryPtr;

		// The header is derived from the PAYLOAD pointer, not the struct address:
		// every non-view instance's payload sits immediately after the struct
		// inside its block ([GcHeader][ObjectInstance][payload]), so a by-value
		// copy of the struct still resolves to the shared header and refcount
		// ops on copies work. The magic distinguishes heap blocks (set) from
		// immortal blocks (zeroed) — see heapSource() for the canonical pointer.
		[[nodiscard]] inline GcHeader* getGcHeader() const
		{
			if (IsView || m_rawMemoryPtr == nullptr)
				return nullptr;

			GcHeader* header = reinterpret_cast<GcHeader*>(static_cast<std::byte*>(m_rawMemoryPtr) - sizeof(ObjectInstance));
			return header->Magic == GcHeader::MAGIC ? header : nullptr;
		}

		[[nodiscard]] inline bool isHeapBacked() const
		{
			return getGcHeader() != nullptr;
		}

		friend class GarbageCollector;

	public:
		MethodSymbol* DelegateTarget = nullptr;
		bool IsStaticRoot = false;
		bool IsView = false;

	public:
		inline ObjectInstance(const TypeSymbol* info, TypeShape* shape, void* memory)
			: m_info(info), m_shape(shape), m_rawMemoryPtr(memory) { }

		~ObjectInstance();

		// Canonical pointer to the instance this value was copied from: valid
		// for heap instances and immortals (payload is always struct-adjacent);
		// nullptr for views and NullInstance. Use when the pointer must outlive
		// the local copy (eval-stack entries, FFI returns, field stores).
		[[nodiscard]] inline ObjectInstance* heapSource() const
		{
			if (IsView || m_rawMemoryPtr == nullptr)
				return nullptr;

			return reinterpret_cast<ObjectInstance*>(static_cast<std::byte*>(m_rawMemoryPtr) - sizeof(ObjectInstance));
		}

		[[nodiscard]] const TypeSymbol* getInfo() const;
		[[nodiscard]] TypeShape* getShape() const;
		[[nodiscard]] void* getMemory() const;
		[[nodiscard]] std::int64_t getReferencesCounter() const;

		// Fields. By-value return: value-type fields yield a borrow view
		// (IsView), reference-type fields yield a copy of the stored struct —
		// refcount ops work on the copy (payload-derived header), and
		// heapSource() recovers the canonical pointer when the identity must
		// outlive the local.
		ObjectInstance GetField(std::uint32_t slot);
		ObjectInstance GetField(const FieldSymbol* field);
		void SetField(std::uint32_t slot, ObjectInstance* instance);
		void SetField(const FieldSymbol* field, ObjectInstance* instance);

		// Arrays — same contract as GetField.
		std::size_t GetArrayLength() const;
		ObjectInstance GetElement(std::size_t index, CallStackFrame* frame = nullptr);
		void SetElement(std::size_t index, ObjectInstance* instance, CallStackFrame* frame = nullptr);
		bool IsInBounds(std::size_t index);

		void IncrementReference();
		void DecrementReference();

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

		void* OffsetMemory(const std::size_t offset, const std::size_t size) const;
		void ReadMemory(const std::size_t offset, const std::size_t size, void* dst) const;
		void WriteMemory(const std::size_t offset, const std::size_t size, const void* src) const;
	};

	/// <summary>
	/// RAII wrapper that keeps an ObjectInstance alive across an async boundary.
	/// </summary>
	struct SHARD_API ObjectRef
	{
		ObjectInstance* Instance = nullptr;

		ObjectRef() = default;
		explicit ObjectRef(ObjectInstance* instance) : Instance(instance)
		{
			if (Instance != nullptr && !Instance->IsNullInstance())
				Instance->IncrementReference();
		}

		~ObjectRef()
		{
			if (Instance != nullptr && !Instance->IsNullInstance())
				Instance->DecrementReference();
		}

		ObjectRef(const ObjectRef&) = delete;
		ObjectRef& operator=(const ObjectRef&) = delete;

		ObjectRef(ObjectRef&& other) noexcept : Instance(other.Instance)
		{
			other.Instance = nullptr;
		}

		ObjectRef& operator=(ObjectRef&& other) noexcept
		{
			if (this != &other)
			{
				if (Instance != nullptr && !Instance->IsNullInstance())
					Instance->DecrementReference();
				Instance = other.Instance;
				other.Instance = nullptr;
			}
			return *this;
		}

		operator ObjectInstance*() const { return Instance; }
		ObjectInstance* operator->() const { return Instance; }
	};

	// Bridges a by-value read into a pointer that outlives the call: views land
	// in a thread_local slot, heap/immortal copies resolve to heapSource(),
	// null yields NullInstance. Transitional until FFI returns stop boxing (S5-I6).
	SHARD_API ObjectInstance* StableRef(ObjectInstance value);
}
