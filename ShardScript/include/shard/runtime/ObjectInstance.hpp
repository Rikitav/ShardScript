#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/syntax/symbols/TypeSymbol.hpp>
#include <shard/syntax/symbols/FieldSymbol.hpp>

#include <string>
#include <cstdint>

namespace shard
{
	class CallStackFrame;

	class SHARD_API ObjectInstance
	{
		const TypeSymbol* Info;
		const bool IsTransient;
		fpos_t ReferencesCounter;
		void* const Memory;

	public:
		inline ObjectInstance(const TypeSymbol* info, void* memory, bool isTransient)
			: Info(info), Memory(memory), IsTransient(isTransient), ReferencesCounter(0) { }
		
		inline ~ObjectInstance() = default;

		const TypeSymbol* getInfo() const;
		void* getMemory() const;
		bool getIsTransient() const;
		fpos_t getReferencesCounter() const;

		ObjectInstance* GetField(FieldSymbol* field, CallStackFrame* frame = nullptr);
		void SetField(FieldSymbol* field, ObjectInstance* instance, CallStackFrame* frame = nullptr);

		ObjectInstance* GetElement(size_t index, CallStackFrame* frame = nullptr);
		void SetElement(size_t index, ObjectInstance* instance, CallStackFrame* frame = nullptr);
		bool IsInBounds(size_t index);

		void IncrementReference();
		void DecrementReference();

		void WriteBoolean(const bool& value) const;
		void WriteInteger(const int64_t& value) const;
		void WriteDouble(const double& value) const;
		void WriteCharacter(const wchar_t& value) const;
		void WriteString(const wchar_t* value) const;
		void WriteString(const wchar_t* value, size_t size) const;
		void WriteString(const std::wstring& value) const;

		bool& AsBoolean() const;
		int64_t& AsInteger() const;
		double& AsDouble() const;
		wchar_t& AsCharacter() const;
		const wchar_t* AsString() const;

		void* OffsetMemory(const size_t offset, const size_t size) const;
		void ReadMemory(const size_t offset, const size_t size, void* dst) const;
		void WriteMemory(const size_t offset, const size_t size, const void* src) const;
	};
}
