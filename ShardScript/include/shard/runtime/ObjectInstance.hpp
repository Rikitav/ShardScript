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

	using ArgumentsSpan = std::span<ObjectInstance*>;

	class SHARD_API ObjectInstance
	{
		const TypeSymbol* m_info;
		TypeShape* m_shape;
		const bool m_isTransient;
		std::int64_t m_eeferencesCounter;
		void* m_rawMemoryPtr;

	public:
		MethodSymbol* DelegateTarget = nullptr;
		bool Terminated = false;
		bool IsSingleton = false;

		// Async task lifetime tracking.
		bool IsTaskLike = false;
		bool IsFireAndForget = false;
		std::shared_ptr<CallStackFrame> FrameOwner;
		void* AsyncNativeState = nullptr;

	public:
		inline ObjectInstance(const TypeSymbol* info, TypeShape* shape, void* memory, bool isTransient)
			: m_info(info), m_shape(shape), m_rawMemoryPtr(memory), m_isTransient(isTransient), m_eeferencesCounter(0) { }
		
		~ObjectInstance();

		void BindToFrame(std::shared_ptr<CallStackFrame> frame);
		void ReleaseFrameOwner();

		[[nodiscard]] const TypeSymbol* getInfo() const;
		[[nodiscard]] TypeShape* getShape() const;
		[[nodiscard]] void* getMemory() const;
		[[nodiscard]] bool getIsTransient() const;
		[[nodiscard]] std::int64_t getReferencesCounter() const;

		// Fields
		ObjectInstance* GetField(std::uint32_t slot);
		void SetField(std::uint32_t slot, ObjectInstance* instance);

		// Arrays
		std::size_t GetArrayLength() const;
		ObjectInstance* GetElement(std::size_t index, CallStackFrame* frame = nullptr);
		void SetElement(std::size_t index, ObjectInstance* instance, CallStackFrame* frame = nullptr);
		bool IsInBounds(std::size_t index);
		ArgumentsSpan ArrayAsSpan();

		void IncrementReference();
		void DecrementReference();

		[[nodiscard]] bool IsNullInstance() const;

		void WriteBoolean(const bool& value) const;
		void WriteInteger(const std::int64_t& value) const;
		void WriteDouble(const double& value) const;
		void WriteCharacter(const wchar_t& value) const;
		void WriteByte(const std::uint8_t& value) const;
		void WriteString(const wchar_t* value) const;
		void WriteString(const wchar_t* value, std::size_t size) const;
		void WriteString(const std::wstring& value) const;

		bool& AsBoolean() const;
		std::int64_t& AsInteger() const;
		double& AsDouble() const;
		wchar_t& AsCharacter() const;
		std::uint8_t& AsByte() const;
		const wchar_t* AsString() const;
		std::int64_t& AsStringLength() const;
		void* AsNint() const;

		void* OffsetMemory(const std::size_t offset, const std::size_t size) const;
		void ReadMemory(const std::size_t offset, const std::size_t size, void* dst) const;
		void WriteMemory(const std::size_t offset, const std::size_t size, const void* src) const;
	};

	/// <summary>RAII wrapper that keeps an ObjectInstance alive across an async boundary.</summary>
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
}
