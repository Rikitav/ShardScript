#include <Windows.h>
#include<iostream>

using namespace std;

namespace shard::utilities
{
	static wstring GetFileVersion()
	{
		TCHAR filename[MAX_PATH];
		GetModuleFileNameW(NULL, filename, MAX_PATH);

		DWORD dummy;
		DWORD size = GetFileVersionInfoSizeW(filename, &dummy);

		wstring result = L"0.0";
		if (size != 0)
		{
			BYTE* versionInfo = new BYTE[size];
			if (GetFileVersionInfoW(filename, 0, size, versionInfo))
			{
				UINT len;
				VS_FIXEDFILEINFO* fileInfo;

				if (VerQueryValueW(versionInfo, TEXT("\\"), (LPVOID*)&fileInfo, &len))
				{
					DWORD versionMS = fileInfo->dwFileVersionMS;
					DWORD versionLS = fileInfo->dwFileVersionLS;
					wstringstream res;

					res << HIWORD(versionMS) << "." << LOWORD(versionMS); // << "." << HIWORD(versionLS) << "." << LOWORD(versionLS);
					result = res.str();
				}
			}

			delete[] versionInfo;
		}

		return result;
	}
}
