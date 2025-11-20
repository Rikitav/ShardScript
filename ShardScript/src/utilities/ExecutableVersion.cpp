#include <shard/ShardScript.h>

#include <Windows.h>
#include <winver.h>
#include <libloaderapi.h>

#pragma comment(lib, "version.lib")

#include <iostream>
#include <sstream>
#include <string>

std::wstring shard::utilities::ShardUtilities::GetFileVersion()
{
	TCHAR filename[MAX_PATH];
	GetModuleFileNameW(NULL, filename, MAX_PATH);

	DWORD dummy;
	DWORD size = GetFileVersionInfoSizeW(filename, &dummy);

	std::wstring result = L"0.0";
	if (size != 0)
	{
		BYTE* versionInfo = new BYTE[size];
		if (GetFileVersionInfoW(filename, 0, size, versionInfo))
		{
			UINT len;
			LPVOID fileInfo = nullptr;
			// cant use VS_FIXEDFILEINFO here just because compiler said so
			
			if (VerQueryValueW(versionInfo, TEXT("\\"), &fileInfo, &len))
			{
				// So im using raw memory access to read that fucking dword
				DWORD versionMS = *(reinterpret_cast<DWORD*>(fileInfo) + 2); // this would normally look like 'fileInfo->dwFileVersionMS;'. but fuck me of course!
				std::wstringstream res;

				res << HIWORD(versionMS) << "." << LOWORD(versionMS); // << "." << HIWORD(versionLS) << "." << LOWORD(versionLS);
				result = res.str();
			}

			delete fileInfo;
		}

		delete[] versionInfo;
	}

	return result;
}
