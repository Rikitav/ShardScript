#pragma once
#include <shard/Definitions.hpp>

#include <filesystem>

namespace gmt
{
	// Cross-platform RAII wrapper over a dynamically loaded shared library.
	class SHARD_API SharedLibrary
	{
		LibraryHandle m_handle = nullptr;
		std::filesystem::path m_path;

	public:
		SharedLibrary() = default;
		explicit SharedLibrary(const std::filesystem::path& path);
		explicit SharedLibrary(LibraryHandle handle);
		~SharedLibrary();

		SharedLibrary(const SharedLibrary&) = delete;
		SharedLibrary& operator=(const SharedLibrary&) = delete;

		SharedLibrary(SharedLibrary&& other) noexcept;
		SharedLibrary& operator=(SharedLibrary&& other) noexcept;

		bool is_loaded() const;
		void load(const std::filesystem::path& path);
		void unload();

		LibraryHandle get_handle() const;
		const std::filesystem::path& get_path() const;

		void* get_function(const char* name) const;

		template<typename T>
		T get_function(const char* name) const
		{
			return reinterpret_cast<T>(get_function(name));
		}
	};
}
