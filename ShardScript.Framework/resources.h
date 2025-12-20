#pragma once
#include <Windows.h>
#include <stdexcept>

namespace resources
{
	typedef HMODULE(*CurrentModuleQuery)(void);

	inline static HMODULE GetCurrentModule()
	{
		HMODULE hModule = nullptr;
		CurrentModuleQuery query = GetCurrentModule;

		GetModuleHandleExW(
			GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
			reinterpret_cast<LPCTSTR>(query),
			&hModule);

		return hModule;
	}

	inline static void GetResource(const wchar_t* name, const wchar_t*& resourceData, size_t& resourceSize)
	{
		HMODULE hModule = GetCurrentModule();
		HRSRC hResource = FindResourceW(hModule, name, L"SOURCE_CODE");
		if (!hResource)
			throw std::runtime_error("");

		HGLOBAL hResourceData = LoadResource(hModule, hResource);
		if (!hResourceData)
			throw std::runtime_error("");

		resourceSize = static_cast<size_t>(SizeofResource(hModule, hResource));
		resourceData = static_cast<wchar_t*>(LockResource(hResourceData));
		FreeLibrary(hModule);
	}
}
