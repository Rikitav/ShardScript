#include <gmt/Strings.hpp>

#include <cwchar>
#include <cstdint>
#include <cstdlib>

#ifdef _WIN32
	#include <Windows.h>
#endif

namespace gmt
{
	std::string wide_to_utf8(std::wstring_view wstr)
	{
		if (wstr.empty())
			return { };

#ifdef _WIN32
		const int size = ::WideCharToMultiByte(
			CP_UTF8, 0,
			wstr.data(), static_cast<int>(wstr.size()),
			nullptr, 0,
			nullptr, nullptr);

		if (size <= 0)
			return { };

		std::string result(static_cast<std::size_t>(size), '\0');
		::WideCharToMultiByte(
			CP_UTF8, 0,
			wstr.data(), static_cast<int>(wstr.size()),
			result.data(), size,
			nullptr, nullptr);

		return result;
#else
		const std::wstring terminated(wstr);
		const std::size_t buffer_size = wstr.size() * 4 + 1;
		std::string result(buffer_size, '\0');

		const std::size_t converted = std::wcstombs(result.data(), terminated.c_str(), buffer_size);
		if (converted == static_cast<std::size_t>(-1))
			return { };

		result.resize(converted);
		return result;
#endif
	}

	std::string wide_to_utf8(const wchar_t* wstr)
	{
		if (wstr == nullptr)
			return { };

		return wide_to_utf8(std::wstring_view(wstr));
	}

	std::wstring utf8_to_wide(std::string_view narrow)
	{
		if (narrow.empty())
			return { };

#ifdef _WIN32
		const int size = ::MultiByteToWideChar(
			CP_UTF8, 0,
			narrow.data(), static_cast<int>(narrow.size()),
			nullptr, 0);

		if (size <= 0)
			return { };

		std::wstring result(static_cast<std::size_t>(size), L'\0');
		::MultiByteToWideChar(
			CP_UTF8, 0,
			narrow.data(), static_cast<int>(narrow.size()),
			result.data(), size);

		return result;
#else
		const std::string terminated(narrow);
		const std::size_t buffer_size = narrow.size() + 1;
		std::wstring result(buffer_size, L'\0');

		const std::size_t converted = std::mbstowcs(result.data(), terminated.c_str(), buffer_size);
		if (converted == static_cast<std::size_t>(-1))
			return { };

		result.resize(converted);
		return result;
#endif
	}

	std::wstring utf8_to_wide(const char* narrow)
	{
		if (narrow == nullptr)
			return { };

		return utf8_to_wide(std::string_view(narrow));
	}

	std::string wide_to_ansi(std::wstring_view wstr)
	{
		if (wstr.empty())
			return { };

#ifdef _WIN32
		const int size = ::WideCharToMultiByte(
			CP_ACP, 0,
			wstr.data(), static_cast<int>(wstr.size()),
			nullptr, 0,
			nullptr, nullptr);

		if (size <= 0)
			return { };

		std::string result(static_cast<std::size_t>(size), '\0');
		::WideCharToMultiByte(
			CP_ACP, 0,
			wstr.data(), static_cast<int>(wstr.size()),
			result.data(), size,
			nullptr, nullptr);

		return result;
#else
		const std::wstring terminated(wstr);
		const std::size_t buffer_size = wstr.size() * 4 + 1;
		std::string result(buffer_size, '\0');

		const std::size_t converted = std::wcstombs(result.data(), terminated.c_str(), buffer_size);
		if (converted == static_cast<std::size_t>(-1))
			return { };

		result.resize(converted);
		return result;
#endif
	}

	std::string wide_to_ansi(const wchar_t* wstr)
	{
		if (wstr == nullptr)
			return { };

		return wide_to_ansi(std::wstring_view(wstr));
	}

	std::wstring ansi_to_wide(std::string_view narrow)
	{
		if (narrow.empty())
			return { };

#ifdef _WIN32
		const int size = ::MultiByteToWideChar(
			CP_ACP, 0,
			narrow.data(), static_cast<int>(narrow.size()),
			nullptr, 0);

		if (size <= 0)
			return { };

		std::wstring result(static_cast<std::size_t>(size), L'\0');
		::MultiByteToWideChar(
			CP_ACP, 0,
			narrow.data(), static_cast<int>(narrow.size()),
			result.data(), size);

		return result;
#else
		const std::string terminated(narrow);
		const std::size_t buffer_size = narrow.size() + 1;
		std::wstring result(buffer_size, L'\0');

		const std::size_t converted = std::mbstowcs(result.data(), terminated.c_str(), buffer_size);
		if (converted == static_cast<std::size_t>(-1))
			return { };

		result.resize(converted);
		return result;
#endif
	}

	std::wstring ansi_to_wide(const char* narrow)
	{
		if (narrow == nullptr)
			return { };

		return ansi_to_wide(std::string_view(narrow));
	}

	std::wstring utf16_to_wide(const void* ptr)
	{
		if (ptr == nullptr)
			return { };

		const auto* source = static_cast<const std::uint16_t*>(ptr);
		std::wstring result;

		while (*source != 0)
		{
			result.push_back(static_cast<wchar_t>(*source));
			++source;
		}

		return result;
	}

	std::string wide_to_narrow(std::wstring_view wstr)
	{
		std::string result;
		result.reserve(wstr.size());

		for (const wchar_t ch : wstr)
			result.push_back(static_cast<char>(ch & 0xFF));

		return result;
	}

	std::string wide_to_narrow(const wchar_t* wstr)
	{
		if (wstr == nullptr)
			return { };

		return wide_to_narrow(std::wstring_view(wstr));
	}
}
