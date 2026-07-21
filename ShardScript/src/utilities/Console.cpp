#include <utilities/Console.hpp>

#if defined(_WIN32)
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>

static void enableFor(DWORD handleId)
{
	HANDLE handle = GetStdHandle(handleId);
	if (handle == INVALID_HANDLE_VALUE)
		return;

	DWORD mode = 0;
	if (!GetConsoleMode(handle, &mode))
		return;

	mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
	SetConsoleMode(handle, mode);
}
#endif

void shard::console::EnableColors()
{
#if defined(_WIN32)
	enableFor(STD_OUTPUT_HANDLE);
	enableFor(STD_ERROR_HANDLE);
#else
	// POSIX terminals interpret ANSI escapes natively; nothing to do.
#endif
}
