#include <shard/runtime/ConsoleHelper.hpp>
#include <shard/runtime/ObjectInstance.hpp>
#include <shard/parsing/semantic/SymbolTable.hpp>

#include <iostream>
#include <consoleapi.h>
#include <processenv.h>
#include <stdexcept>
#include <string>
#include <wchar.h>
#include <cstdint>

#if defined(_MSC_VER)
#include <windows.h>
#define USEWIN 1
#endif

using namespace shard;

static const HANDLE stdOut = GetStdHandle(STD_OUTPUT_HANDLE);

void ConsoleHelper::Write(ObjectInstance* instance)
{
	if (instance->Info == SymbolTable::Primitives::Boolean)
	{
		bool data = instance->AsBoolean();
		Write(data);
		return;
	}

	if (instance->Info == SymbolTable::Primitives::Integer)
	{
		int64_t data = instance->AsInteger();
		Write(data);
		return;
	}

	if (instance->Info == SymbolTable::Primitives::Double)
	{
		double data = instance->AsDouble();
		Write(data);
		return;
	}

	if (instance->Info == SymbolTable::Primitives::Char)
	{
		wchar_t data = instance->AsCharacter();
		Write(data);
		return;
	}

	if (instance->Info == SymbolTable::Primitives::String)
	{
		std::wstring data = instance->AsString();
		Write(data);
		return;
	}

	if (instance->Info->FullName.capacity() > 0)
	{
		Write(instance->Info->FullName);
		return;
	}

	if (instance->Info->Name.capacity() > 0)
	{
		Write(instance->Info->Name);
		return;
	}

	throw std::runtime_error("unknown type");
}

void ConsoleHelper::Write(bool data)
{
	const wchar_t* text = data ? L"true" : L"false";
	Write(text);
}

void ConsoleHelper::Write(int64_t data)
{
	std::wstring strData = std::to_wstring(data);
	Write(strData);
}

void ConsoleHelper::Write(double data)
{
	std::wstring strData = std::to_wstring(data);
	Write(strData);
}

void ConsoleHelper::Write(wchar_t data)
{
	DWORD charsWritten;
#if USEWIN
	WriteConsoleW(stdOut, &data, sizeof(wchar_t), &charsWritten, NULL);
#else
	wprintf(L"%ls", data);
	fflush(stdout);
#endif
}

void ConsoleHelper::Write(const wchar_t* data)
{
	DWORD charsWritten;
#if USEWIN
	WriteConsoleW(stdOut, data, static_cast<DWORD>(wcslen(data)), &charsWritten, NULL);
#else
	wprintf(L"%ls", data);
	fflush(stdout);
#endif
}

void ConsoleHelper::Write(const std::wstring& data)
{
	DWORD charsWritten;
	const wchar_t* text = data.c_str();

#if USEWIN
	WriteConsoleW(stdOut, text, static_cast<DWORD>(data.size()), &charsWritten, NULL);
#else
	wprintf(L"%ls", text);
	fflush(stdout);
#endif
}

void ConsoleHelper::WriteLine(ObjectInstance* instance)
{
	Write(instance);
	WriteLine();
}

void ConsoleHelper::WriteLine()
{
	std::cout << std::endl;
}

void ConsoleHelper::WriteLine(bool data)
{
	Write(data);
	std::cout << std::endl;
}

void ConsoleHelper::WriteLine(int64_t data)
{
	Write(data);
	std::cout << std::endl;
}

void ConsoleHelper::WriteLine(double data)
{
	Write(data);
	std::cout << std::endl;
}

void ConsoleHelper::WriteLine(wchar_t data)
{
	Write(data);
	std::cout << std::endl;
}

void ConsoleHelper::WriteLine(const wchar_t* data)
{
	Write(data);
	std::cout << std::endl;
}

void ConsoleHelper::WriteLine(const std::wstring& data)
{
	Write(data);
	std::cout << std::endl;
}
