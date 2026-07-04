#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/semantic/symbols/TypeSymbol.hpp>
#include <shard/semantic/symbols/FieldSymbol.hpp>

#include <string>
#include <cstdint>
#include <span>

namespace shard
{
	class CallStackFrame;
	class MethodSymbol;
	class ObjectInstance;

	using ArgumentsSpan = std::span<ObjectInstance*>;

	class SHARD_API ObjectInstance
	{
		const TypeSymbol* m_info;
		const bool m_isTransient;
		std::int64_t m_eeferencesCounter;
		void* m_rawMemoryPtr;

	public:
		MethodSymbol* DelegateTarget = nullptr;
		bool Terminated = false;

	public:
		inline ObjectInstance(const TypeSymbol* info, void* memory, bool isTransient)
			: m_info(info), m_rawMemoryPtr(memory), m_isTransient(isTransient), m_eeferencesCounter(0) { }
		
		inline ~ObjectInstance() = default;

		[[nodiscard]] const TypeSymbol* getInfo() const;
		[[nodiscard]] void* getMemory() const;
		[[nodiscard]] bool getIsTransient() const;
		[[nodiscard]] std::int64_t getReferencesCounter() const;

		// Fields
		ObjectInstance* GetField(FieldSymbol* field, CallStackFrame* frame = nullptr);
		void SetField(FieldSymbol* field, ObjectInstance* instance, CallStackFrame* frame = nullptr);

		// Arrays
		std::size_t GetArrayLength() const;
		ObjectInstance* GetElement(std::size_t index, CallStackFrame* frame = nullptr);
		void SetElement(std::size_t index, ObjectInstance* instance, CallStackFrame* frame = nullptr);
		bool IsInBounds(std::size_t index);
		ArgumentsSpan ArrayAsSpan();

		void IncrementReference();
		void DecrementReference();

		void WriteBoolean(const bool& value) const;
		void WriteInteger(const std::int64_t& value) const;
		void WriteDouble(const double& value) const;
		void WriteCharacter(const wchar_t& value) const;
		void WriteString(const wchar_t* value) const;
		void WriteString(const wchar_t* value, std::size_t size) const;
		void WriteString(const std::wstring& value) const;

		bool& AsBoolean() const;
		std::int64_t& AsInteger() const;
		double& AsDouble() const;
		wchar_t& AsCharacter() const;
		const wchar_t* AsString() const;
		std::int64_t& AsStringLength() const;
		void* AsNint() const;

		void* OffsetMemory(const std::size_t offset, const std::size_t size) const;
		void ReadMemory(const std::size_t offset, const std::size_t size, void* dst) const;
		void WriteMemory(const std::size_t offset, const std::size_t size, const void* src) const;
	};
}
