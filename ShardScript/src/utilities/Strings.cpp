#include <utilities/Strings.hpp>

#include <cwchar>
#include <cstdlib>

#ifdef _WIN32
#   ifndef NOMINMAX
#       define NOMINMAX
#   endif
#   ifndef WIN32_LEAN_AND_MEAN
#       define WIN32_LEAN_AND_MEAN
#   endif
#   include <Windows.h>
#endif

namespace shard::strings
{
    std::string WideToUtf8(const std::wstring& wstr)
    {
        if (wstr.empty())
            return {};

#ifdef _WIN32
        const int size = ::WideCharToMultiByte(
            CP_UTF8, 0,
            wstr.data(), static_cast<int>(wstr.size()),
            nullptr, 0,
            nullptr, nullptr);

        if (size <= 0)
            return {};

        std::string result(static_cast<std::size_t>(size), '\0');
        ::WideCharToMultiByte(
            CP_UTF8, 0,
            wstr.data(), static_cast<int>(wstr.size()),
            result.data(), size,
            nullptr, nullptr);

        return result;
#else
        const std::size_t bufferSize = wstr.size() * 4 + 1;
        std::string result(bufferSize, '\0');

        const std::size_t converted = std::wcstombs(result.data(), wstr.c_str(), bufferSize);
        if (converted == static_cast<std::size_t>(-1))
            return {};

        result.resize(converted);
        return result;
#endif
    }

    std::string WideToUtf8(const wchar_t* wstr)
    {
        if (wstr == nullptr)
            return {};
        return WideToUtf8(std::wstring(wstr));
    }

    std::wstring strings::Utf8ToWide(const std::string& narrow)
    {
        if (narrow.empty())
            return {};

#ifdef _WIN32
        const int size = ::MultiByteToWideChar(
            CP_UTF8, 0,
            narrow.data(), static_cast<int>(narrow.size()),
            nullptr, 0);

        if (size <= 0)
            return {};

        std::wstring result(static_cast<std::size_t>(size), L'\0');
        ::MultiByteToWideChar(
            CP_UTF8, 0,
            narrow.data(), static_cast<int>(narrow.size()),
            result.data(), size);

        return result;
#else
        const std::size_t bufferSize = narrow.size() + 1;
        std::wstring result(bufferSize, L'\0');

        const std::size_t converted = std::mbstowcs(result.data(), narrow.c_str(), bufferSize);
        if (converted == static_cast<std::size_t>(-1))
            return {};

        result.resize(converted);
        return result;
#endif
    }

    std::wstring strings::Utf8ToWide(const char* narrow)
    {
        if (narrow == nullptr)
            return {};

        return strings::Utf8ToWide(std::string(narrow));
    }

    std::string WideToAnsi(const std::wstring& wstr)
    {
        if (wstr.empty())
            return {};

#ifdef _WIN32
        const int size = ::WideCharToMultiByte(
            CP_ACP, 0,
            wstr.data(), static_cast<int>(wstr.size()),
            nullptr, 0,
            nullptr, nullptr);

        if (size <= 0)
            return {};

        std::string result(static_cast<std::size_t>(size), '\0');
        ::WideCharToMultiByte(
            CP_ACP, 0,
            wstr.data(), static_cast<int>(wstr.size()),
            result.data(), size,
            nullptr, nullptr);

        return result;
#else
        const std::size_t bufferSize = wstr.size() * 4 + 1;
        std::string result(bufferSize, '\0');

        const std::size_t converted = std::wcstombs(result.data(), wstr.c_str(), bufferSize);
        if (converted == static_cast<std::size_t>(-1))
            return {};

        result.resize(converted);
        return result;
#endif
    }

    std::string WideToAnsi(const wchar_t* wstr)
    {
        if (wstr == nullptr)
            return {};
        return WideToAnsi(std::wstring(wstr));
    }

    std::wstring AnsiToWide(const std::string& narrow)
    {
        if (narrow.empty())
            return {};

#ifdef _WIN32
        const int size = ::MultiByteToWideChar(
            CP_ACP, 0,
            narrow.data(), static_cast<int>(narrow.size()),
            nullptr, 0);

        if (size <= 0)
            return {};

        std::wstring result(static_cast<std::size_t>(size), L'\0');
        ::MultiByteToWideChar(
            CP_ACP, 0,
            narrow.data(), static_cast<int>(narrow.size()),
            result.data(), size);

        return result;
#else
        const std::size_t bufferSize = narrow.size() + 1;
        std::wstring result(bufferSize, L'\0');

        const std::size_t converted = std::mbstowcs(result.data(), narrow.c_str(), bufferSize);
        if (converted == static_cast<std::size_t>(-1))
            return {};

        result.resize(converted);
        return result;
#endif
    }

    std::wstring AnsiToWide(const char* narrow)
    {
        if (narrow == nullptr)
            return {};
        return AnsiToWide(std::string(narrow));
    }

    std::wstring Utf16ToWide(const void* ptr)
    {
        if (ptr == nullptr)
            return {};

        const auto* source = static_cast<const std::uint16_t*>(ptr);
        std::wstring result;

        while (*source != 0)
        {
            result.push_back(static_cast<wchar_t>(*source));
            ++source;
        }

        return result;
    }

    std::string WideToNarrow(const std::wstring& wstr)
    {
        std::string result;
        result.reserve(wstr.size());

        for (const wchar_t ch : wstr)
            result.push_back(static_cast<char>(ch & 0xFF));

        return result;
    }

    std::string WideToNarrow(const wchar_t* wstr)
    {
        if (wstr == nullptr)
            return {};
        return WideToNarrow(std::wstring(wstr));
    }
} // namespace shard::strings
