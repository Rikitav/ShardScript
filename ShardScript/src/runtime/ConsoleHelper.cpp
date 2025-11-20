#include <shard/runtime/ConsoleHelper.h>
#include <shard/runtime/ObjectInstance.h>
#include <shard/parsing/semantic/SymbolTable.h>

#include <windows.h>
#include <iostream>
#include <malloc.h>
#include <consoleapi.h>
#include <processenv.h>
#include <cstdio>
#include <stdexcept>
#include <string>

using namespace shard::runtime;
using namespace shard::parsing::semantic;

static const HANDLE stdOut = GetStdHandle(STD_OUTPUT_HANDLE);

void ConsoleHelper::Write(ObjectInstance* instance)
{
	if (instance->Info == SymbolTable::Primitives::Boolean)
	{
		bool data = instance->ReadPrimitive<bool>();
		Write(data);
		return;
	}

	if (instance->Info == SymbolTable::Primitives::Integer)
	{
		int data = instance->ReadPrimitive<int>();
		Write(data);
		return;
	}

	if (instance->Info == SymbolTable::Primitives::Char)
	{
		wchar_t data = instance->ReadPrimitive<wchar_t>();
		Write(data);
		return;
	}

	if (instance->Info == SymbolTable::Primitives::String)
	{
		std::wstring data = instance->ReadPrimitive<std::wstring>();
		Write(data);
		return;
	}

	throw std::runtime_error("unknown type");
}

void ConsoleHelper::Write(bool data)
{
	DWORD charsWritten;
	const wchar_t* text = data ? L"true" : L"false";
	WriteConsoleW(stdOut, text, static_cast<DWORD>(wcslen(text)), &charsWritten, NULL);
}

void ConsoleHelper::Write(int data)
{
	int needed_size = _scwprintf(L"%d", data) + 1;
	if (needed_size <= 0)
		return;

	wchar_t* buffer = (wchar_t*)malloc(needed_size * sizeof(wchar_t));
	if (!buffer)
		return;

	DWORD charsWritten;
	swprintf_s(buffer, needed_size, L"%d", data);
	WriteConsoleW(stdOut, buffer, static_cast<DWORD>(wcslen(buffer)), &charsWritten, NULL);
	free(buffer);
}

void ConsoleHelper::Write(wchar_t data)
{
	DWORD charsWritten;
	WriteConsoleW(stdOut, &data, sizeof(wchar_t), &charsWritten, NULL);
}

void ConsoleHelper::Write(const wchar_t* data)
{
	DWORD charsWritten;
	WriteConsoleW(stdOut, data, static_cast<DWORD>(wcslen(data)), &charsWritten, NULL);
}

void ConsoleHelper::Write(std::wstring data)
{
	DWORD charsWritten;
	const wchar_t* text = data.c_str();
	WriteConsoleW(stdOut, text, static_cast<DWORD>(wcslen(text)), &charsWritten, NULL);
}

void ConsoleHelper::WriteLine(ObjectInstance* instance)
{
	if (instance->Info == SymbolTable::Primitives::Boolean)
	{
		bool data = instance->ReadPrimitive<bool>();
		WriteLine(data);
		return;
	}

	if (instance->Info == SymbolTable::Primitives::Integer)
	{
		int data = instance->ReadPrimitive<int>();
		WriteLine(data);
		return;
	}

	if (instance->Info == SymbolTable::Primitives::Char)
	{
		wchar_t data = instance->ReadPrimitive<wchar_t>();
		WriteLine(data);
		return;
	}

	if (instance->Info == SymbolTable::Primitives::String)
	{
		std::wstring data = instance->ReadPrimitive<std::wstring>();
		WriteLine(data);
		return;
	}

	throw std::runtime_error("unknown type");
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

void ConsoleHelper::WriteLine(int data)
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

void ConsoleHelper::WriteLine(std::wstring data)
{
	Write(data);
	std::cout << std::endl;
}
