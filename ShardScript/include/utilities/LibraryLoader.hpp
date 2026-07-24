#pragma once

#include <shard/ShardScriptAPI.hpp>

#include <filesystem>
#include <string>

namespace shard::utilities
{
    /// <summary>
    /// Cross-platform RAII wrapper for a dynamically loaded shared library.
    /// </summary>
    class SHARD_API SharedLibrary
    {
    public:
        SharedLibrary() = default;

        explicit SharedLibrary(const std::filesystem::path& path);
        explicit SharedLibrary(LibraryHandle handle);

        ~SharedLibrary();

        SharedLibrary(const SharedLibrary&) = delete;
        SharedLibrary& operator=(const SharedLibrary&) = delete;

        SharedLibrary(SharedLibrary&& other) noexcept;
        SharedLibrary& operator=(SharedLibrary&& other) noexcept;

        bool IsLoaded() const;
        void Load(const std::filesystem::path& path);
        void Unload();

        LibraryHandle GetHandle() const;

        const std::filesystem::path& GetPath() const;

        void* GetFunction(const char* name) const;

        template<typename T>
        T GetFunction(const char* name) const
        {
            return reinterpret_cast<T>(GetFunction(name));
        }

    private:
        LibraryHandle Handle = nullptr;
        std::filesystem::path Path;
    };
}
