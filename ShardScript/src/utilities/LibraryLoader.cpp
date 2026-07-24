#include <utilities/LibraryLoader.hpp>
#include <utilities/Strings.hpp>

#include <stdexcept>
#include <sstream>

#if defined(_WIN32)
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#elif defined(__linux__)
    #include <dlfcn.h>
#elif defined(__APPLE__)
    #include <dlfcn.h>
#endif

namespace shard::utilities
{
    static std::string BuildLoadError(const std::filesystem::path& path, const std::wstring& systemMessage)
    {
        std::ostringstream oss;
        oss << "Failed to load library \"" << strings::WideToUtf8(path.wstring()) << "\"";
        if (!systemMessage.empty())
            oss << ": " << strings::WideToUtf8(systemMessage);
        return oss.str();
    }

    static std::string BuildLoadError(const std::filesystem::path& path, const char* systemMessage)
    {
        std::ostringstream oss;
        oss << "Failed to load library \"" << strings::WideToUtf8(path.wstring()) << "\"";
        if (systemMessage != nullptr && systemMessage[0] != '\0')
            oss << ": " << systemMessage;
        return oss.str();
    }

#if defined(_WIN32)
    static std::wstring GetLastErrorMessage()
    {
        DWORD errorMessageID = ::GetLastError();
        if (errorMessageID == 0)
            return std::wstring();

        LPWSTR messageBuffer = nullptr;
        std::size_t size = ::FormatMessageW(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            nullptr, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            reinterpret_cast<LPWSTR>(&messageBuffer), 0, nullptr);

        std::wstring message(messageBuffer, size);
        ::LocalFree(messageBuffer);
        return message;
    }
#endif

    SharedLibrary::SharedLibrary(const std::filesystem::path& path)
    {
        Load(path);
    }

    SharedLibrary::SharedLibrary(LibraryHandle handle)
        : Handle(handle)
    {
    }

    SharedLibrary::~SharedLibrary()
    {
        Unload();
    }

    SharedLibrary::SharedLibrary(SharedLibrary&& other) noexcept
        : Handle(other.Handle), Path(std::move(other.Path))
    {
        other.Handle = nullptr;
    }

    SharedLibrary& SharedLibrary::operator=(SharedLibrary&& other) noexcept
    {
        if (this != &other)
        {
            Unload();
            Handle = other.Handle;
            Path = std::move(other.Path);
            other.Handle = nullptr;
        }
        return *this;
    }

    bool SharedLibrary::IsLoaded() const
    {
        return Handle != nullptr;
    }

    void SharedLibrary::Load(const std::filesystem::path& path)
    {
        Unload();
        Path = path;

#if defined(_WIN32)
        Handle = static_cast<LibraryHandle>(::LoadLibraryW(path.c_str()));
        if (Handle == nullptr)
            throw std::runtime_error(BuildLoadError(path, GetLastErrorMessage()));
#else
        Handle = ::dlopen(path.c_str(), RTLD_NOW);
        if (Handle == nullptr)
            throw std::runtime_error(BuildLoadError(path, ::dlerror()));
#endif
    }

    void SharedLibrary::Unload()
    {
        if (Handle == nullptr)
            return;

#if defined(_WIN32)
        ::FreeLibrary(static_cast<HMODULE>(Handle));
#else
        ::dlclose(Handle);
#endif
        Handle = nullptr;
        Path.clear();
    }

    LibraryHandle SharedLibrary::GetHandle() const
    {
        return Handle;
    }

    const std::filesystem::path& SharedLibrary::GetPath() const
    {
        return Path;
    }

    void* SharedLibrary::GetFunction(const char* name) const
    {
        if (Handle == nullptr || name == nullptr)
            return nullptr;

#if defined(_WIN32)
        return reinterpret_cast<void*>(::GetProcAddress(static_cast<HMODULE>(Handle), name));
#else
        return ::dlsym(Handle, name);
#endif
    }
}
