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
		const TypeSymbol* Info;
		const bool IsTransient;
		std::int64_t ReferencesCounter;
		void* Memory;
		std::size_t ArrayLength = 0;
		std::size_t ArrayMemorySize = 0;

	public:
		MethodSymbol* DelegateTarget = nullptr;
		bool Terminated = false;

	public:
		inline ObjectInstance(const TypeSymbol* info, void* memory, bool isTransient)
			: Info(info), Memory(memory), IsTransient(isTransient), ReferencesCounter(0)
		{
			if (info != nullptr)
				ArrayMemorySize = info->MemoryBytesSize;
		}
		
		inline ~ObjectInstance() = default;

		inline std::size_t GetArrayLength() const { return ArrayLength; }
		inline void SetArrayLength(std::size_t length) { ArrayLength = length; }
		inline std::size_t GetMemorySize() const { return ArrayMemorySize; }
		inline void SetMemorySize(std::size_t size) { ArrayMemorySize = size; }

		inline const TypeSymbol* getInfo() const;
		inline void* getMemory() const;
		inline bool getIsTransient() const;
		inline std::int64_t getReferencesCounter() const;

		ObjectInstance* GetField(FieldSymbol* field, CallStackFrame* frame = nullptr);
		void SetField(FieldSymbol* field, ObjectInstance* instance, CallStackFrame* frame = nullptr);

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
		std::int64_t& AsStringLength() const;
		const wchar_t* AsString() const;
		void* AsNint() const;

		void* OffsetMemory(const std::size_t offset, const std::size_t size) const;
		void ReadMemory(const std::size_t offset, const std::size_t size, void* dst) const;
		void WriteMemory(const std::size_t offset, const std::size_t size, const void* src) const;
	};
}
