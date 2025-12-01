#include <Windows.h>
#include <string>
#include <stdexcept>

#include "InterpreterUtilities.h"

const std::wstring FileTypeExtension = L".ss";
const std::wstring FileTypeId = L"ShardLang";
const std::wstring OpenCommandKey = FileTypeExtension + L"\\shell\\open\\command";
const std::wstring FileTypeDescription = L"ShardScript source code file";

static std::wstring GetCurrentExecutablePath()
{
	wchar_t path[MAX_PATH];
	GetModuleFileNameW(NULL, path, MAX_PATH);
	return std::wstring(path);
}

static bool IsRunningAsAdmin()
{
	BOOL isAdmin = FALSE;
	PSID adminGroup = NULL;

	SID_IDENTIFIER_AUTHORITY ntAuth = SECURITY_NT_AUTHORITY;
	if (AllocateAndInitializeSid(&ntAuth, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &adminGroup))
	{
		if (!CheckTokenMembership(NULL, adminGroup, &isAdmin))
			isAdmin = FALSE;

		FreeSid(adminGroup);
	}

	return isAdmin != FALSE;
}

static bool CreateExtensionAssociation()
{
	HKEY hKey;
	LONG result = RegCreateKeyExW(HKEY_CLASSES_ROOT,
		FileTypeExtension.c_str(), 0, NULL,
		REG_OPTION_NON_VOLATILE, KEY_WRITE,
		NULL, &hKey, NULL);
	
	if (result != ERROR_SUCCESS)
		throw std::runtime_error("Failed to open registry key");

	const BYTE* cstr = reinterpret_cast<const BYTE*>(FileTypeId.c_str());
	DWORD size = static_cast<DWORD>((FileTypeId.length() + 1) * sizeof(wchar_t));
	
	result = RegSetValueExW(hKey, L"", 0, REG_SZ, cstr, size);
	RegCloseKey(hKey);

	if (result != ERROR_SUCCESS)
		throw std::runtime_error("Failed to write file type");

	return true;
}

static bool CreateFileTypeDescription()
{
	HKEY hKey;
	LONG result = RegCreateKeyExW(HKEY_CLASSES_ROOT,
		FileTypeExtension.c_str(), 0, NULL,
		REG_OPTION_NON_VOLATILE, KEY_WRITE,
		NULL, &hKey, NULL);

	if (result != ERROR_SUCCESS)
		throw std::runtime_error("Failed to open registry key");

	const BYTE* cstr = reinterpret_cast<const BYTE*>(FileTypeDescription.c_str());
	DWORD size = static_cast<DWORD>((FileTypeDescription.length() + 1) * sizeof(wchar_t));

	result = RegSetValueExW(hKey, L"", 0, REG_SZ, cstr, size);
	RegCloseKey(hKey);

	if (result != ERROR_SUCCESS)
		throw std::runtime_error("Failed to write file type");

	return true;
}

static bool CreateOpenCommand()
{
	HKEY hKey;
	LONG result = RegCreateKeyExW(HKEY_CLASSES_ROOT,
		OpenCommandKey.c_str(), 0, NULL,
		REG_OPTION_NON_VOLATILE, KEY_WRITE,
		NULL, &hKey, NULL);

	if (result != ERROR_SUCCESS)
		throw std::runtime_error("Failed to open registry key");

	std::wstring path = GetCurrentExecutablePath() + L" \"%1\"";
	const BYTE* cstr = reinterpret_cast<const BYTE*>(path.c_str());
	DWORD size = static_cast<DWORD>((path.length() + 1) * sizeof(wchar_t));

	result = RegSetValueExW(hKey, L"", 0, REG_SZ, cstr, size);
	RegCloseKey(hKey);

	if (result != ERROR_SUCCESS)
		throw std::runtime_error("Failed to write file type");

	return true;
}

void shard::interpreter::utilities::ShardUtilities::AssociateRegistry()
{
	if (!CreateExtensionAssociation())
		return;

	/*
	if (!CreateFileTypeDescription())
		return;
	*/

	if (!CreateOpenCommand())
		return;
}
