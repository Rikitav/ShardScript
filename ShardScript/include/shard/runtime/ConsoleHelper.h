#pragma once
#include <shard/runtime/ObjectInstance.h>
#include <string>

namespace shard::runtime
{
	class ConsoleHelper
	{
	public:
		static void Write(ObjectInstance* instance);
		static void Write(bool data);
		static void Write(int data);
		static void Write(wchar_t data);
		static void Write(std::wstring data);

		static void WriteLine(ObjectInstance* instance);
		static void WriteLine(bool data);
		static void WriteLine(int data);
		static void WriteLine(wchar_t data);
		static void WriteLine(std::wstring data);
	};
}