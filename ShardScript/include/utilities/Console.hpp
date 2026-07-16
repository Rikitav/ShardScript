#pragma once

#include <cwchar>

namespace shard::console
{
	constexpr const wchar_t* BG_BLACK = L"\x1b[40m";
	constexpr const wchar_t* BG_DARK_RED = L"\x1b[41m";
	constexpr const wchar_t* BG_DARK_GREEN = L"\x1b[42m";
	constexpr const wchar_t* BG_DARK_YELLOW = L"\x1b[43m";
	constexpr const wchar_t* BG_DARK_BLUE = L"\x1b[44m";
	constexpr const wchar_t* BG_DARK_MAGENTA = L"\x1b[45m";
	constexpr const wchar_t* BG_DARK_CYAN = L"\x1b[46m";
	constexpr const wchar_t* BG_LIGHT_GRAY = L"\x1b[47m";
	constexpr const wchar_t* BG_DARK_GRAY = L"\x1b[100m";
	constexpr const wchar_t* BG_RED = L"\x1b[101m";
	constexpr const wchar_t* BG_GREEN = L"\x1b[102m";
	constexpr const wchar_t* BG_YELLOW = L"\x1b[103m";
	constexpr const wchar_t* BG_BLUE = L"\x1b[104m";
	constexpr const wchar_t* BG_MAGENTA = L"\x1b[105m";
	constexpr const wchar_t* BG_CYAN = L"\x1b[106m";
	constexpr const wchar_t* BG_WHITE = L"\x1b[107m";

	constexpr const wchar_t* FG_BLACK = L"\x1b[30m";
	constexpr const wchar_t* FG_DARK_RED = L"\x1b[31m";
	constexpr const wchar_t* FG_DARK_GREEN = L"\x1b[32m";
	constexpr const wchar_t* FG_DARK_YELLOW = L"\x1b[33m";
	constexpr const wchar_t* FG_DARK_BLUE = L"\x1b[34m";
	constexpr const wchar_t* FG_DARK_MAGENTA = L"\x1b[35m";
	constexpr const wchar_t* FG_DARK_CYAN = L"\x1b[36m";
	constexpr const wchar_t* FG_LIGHT_GRAY = L"\x1b[37m";
	constexpr const wchar_t* FG_DARK_GRAY = L"\x1b[90m";
	constexpr const wchar_t* FG_RED = L"\x1b[91m";
	constexpr const wchar_t* FG_GREEN = L"\x1b[92m";
	constexpr const wchar_t* FG_YELLOW = L"\x1b[93m";
	constexpr const wchar_t* FG_BLUE = L"\x1b[94m";
	constexpr const wchar_t* FG_MAGENTA = L"\x1b[95m";
	constexpr const wchar_t* FG_CYAN = L"\x1b[96m";
	constexpr const wchar_t* FG_WHITE = L"\x1b[97m";

	constexpr const wchar_t* ST_BOLD = L"\x1B[1m";
	constexpr const wchar_t* ST_RESET = L"\x1B[0m";

    void EnableColors();
}
