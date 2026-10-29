#include <gmt/Containers.hpp>
#include <gmt/SharedLibrary.hpp>

#include <gmt/Strings.hpp>
#include <gmt/Concat.hpp>

#include <stdexcept>

#if defined(_WIN32)
	#include <windows.h>
#elif defined(__EMSCRIPTEN__)
	// WebAssembly does not support native dynamic library loading
#else
	#include <dlfcn.h>
#endif

using namespace gmt;

namespace
{
	gmt::string build_load_error(const std::filesystem::path& path, const gmt::string& systemMessage)
	{
		gmt::string message = gmt::wide_to_utf8(gmt::concat(L"Failed to load library \"", path.wstring(), L"\""));
		if (!systemMessage.empty())
			message += ": " + systemMessage;

		return message;
	}

#if defined(_WIN32)
	gmt::string get_last_error_message()
	{
		DWORD errorMessageID = ::GetLastError();
		if (errorMessageID == 0)
			return gmt::string();

		LPWSTR messageBuffer = nullptr;
		std::size_t size = ::FormatMessageW(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			nullptr, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			reinterpret_cast<LPWSTR>(&messageBuffer), 0, nullptr);

		gmt::string message = gmt::wide_to_utf8(std::wstring_view(messageBuffer, size));
		::LocalFree(messageBuffer);
		return message;
	}
#endif
}

SharedLibrary::SharedLibrary(const std::filesystem::path& path)
{
	load(path);
}

SharedLibrary::SharedLibrary(LibraryHandle handle)
	: m_handle(handle)
{ }

SharedLibrary::~SharedLibrary()
{
	unload();
}

SharedLibrary::SharedLibrary(SharedLibrary&& other) noexcept
	: m_handle(other.m_handle),
	  m_path(std::move(other.m_path))
{
	other.m_handle = nullptr;
}

SharedLibrary& SharedLibrary::operator=(SharedLibrary&& other) noexcept
{
	if (this != &other)
	{
		unload();
		m_handle = other.m_handle;
		m_path = std::move(other.m_path);
		other.m_handle = nullptr;
	}

	return *this;
}

bool SharedLibrary::is_loaded() const
{
	return m_handle != nullptr;
}

void SharedLibrary::load(const std::filesystem::path& path)
{
	unload();
	m_path = path;

#if defined(_WIN32)
	m_handle = static_cast<LibraryHandle>(::LoadLibraryW(path.c_str()));
	if (m_handle == nullptr)
		throw std::runtime_error(build_load_error(path, get_last_error_message()).c_str());

#elif defined(__EMSCRIPTEN__)
	throw std::runtime_error(build_load_error(path, "native library loading is not supported in WebAssembly").c_str());

#else
	m_handle = ::dlopen(path.c_str(), RTLD_NOW);
	if (m_handle == nullptr)
	{
		const char* error = ::dlerror();
		throw std::runtime_error(build_load_error(path, error != nullptr ? error : "").c_str());
	}
#endif
}

void SharedLibrary::unload()
{
	if (m_handle == nullptr)
		return;

#if defined(_WIN32)
	::FreeLibrary(static_cast<HMODULE>(m_handle));

#elif !defined(__EMSCRIPTEN__)
	::dlclose(m_handle);

#endif

	m_handle = nullptr;
	m_path.clear();
}

LibraryHandle SharedLibrary::get_handle() const
{
	return m_handle;
}

const std::filesystem::path& SharedLibrary::get_path() const
{
	return m_path;
}

void* SharedLibrary::get_function(const char* name) const
{
	if (m_handle == nullptr || name == nullptr)
		return nullptr;

#if defined(_WIN32)
	return reinterpret_cast<void*>(::GetProcAddress(static_cast<HMODULE>(m_handle), name));
#elif defined(__EMSCRIPTEN__)
	return nullptr;
#else
	return ::dlsym(m_handle, name);
#endif
}
