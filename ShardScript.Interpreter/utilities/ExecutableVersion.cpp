#include <Windows.h>
#include <libloaderapi.h>
#include <iostream>
#include <sstream>
#include <string>

#pragma comment(lib, "version.lib")

#include "InterpreterUtilities.h"

std::wstring shard::ShardUtilities::GetFileVersion()
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
			VS_FIXEDFILEINFO* fileInfo = nullptr;
			
			if (VerQueryValueW(versionInfo, TEXT("\\"), reinterpret_cast<LPVOID*>(&fileInfo), &len))
			{
				DWORD versionMS = fileInfo->dwFileVersionMS;
				std::wstringstream res;

				res << HIWORD(versionMS) << "." << LOWORD(versionMS); // << "." << HIWORD(versionLS) << "." << LOWORD(versionLS);
				result = res.str();
			}
		}

		delete[] versionInfo;
	}

	return result;
}
