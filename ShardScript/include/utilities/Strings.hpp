#pragma once

#include <string>

namespace shard::strings
{
    /// Converts a wide string to UTF-8.
    std::string WideToUtf8(const std::wstring& wstr);
    std::string WideToUtf8(const wchar_t* wstr);

    /// Converts a UTF-8 string to wide.
    std::wstring Utf8ToWide(const std::string& narrow);
    std::wstring Utf8ToWide(const char* narrow);

    /// Converts a wide string to the active code page (ANSI).
    std::string WideToAnsi(const std::wstring& wstr);
    std::string WideToAnsi(const wchar_t* wstr);

    /// Converts an ANSI string to wide.
    std::wstring AnsiToWide(const std::string& narrow);
    std::wstring AnsiToWide(const char* narrow);

    /// Converts a NUL-terminated UTF-16 buffer to a wide string.
    std::wstring Utf16ToWide(const void* ptr);

    /// Truncates a wide string to single-byte characters (mask 0xFF).
    /// Useful for ASCII-only symbol names where full conversion is unnecessary.
    std::string WideToNarrow(const std::wstring& wstr);
    std::string WideToNarrow(const wchar_t* wstr);
} // namespace shard::strings
