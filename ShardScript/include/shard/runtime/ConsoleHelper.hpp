#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/runtime/ObjectInstance.hpp>
#include <string>

namespace shard
{
	class SHARD_API ConsoleHelper
	{
	public:
		static void Write(ObjectInstance* instance);
		static void Write(bool data);
		static void Write(int64_t data);
		static void Write(double data);
		static void Write(wchar_t data);
		static void Write(const wchar_t* data);
		static void Write(std::wstring data);

		static void WriteLine(ObjectInstance* instance);
		static void WriteLine();
		static void WriteLine(bool data);
		static void WriteLine(int64_t data);
		static void WriteLine(double data);
		static void WriteLine(wchar_t data);
		static void WriteLine(const wchar_t* data);
		static void WriteLine(std::wstring data);
	};
}