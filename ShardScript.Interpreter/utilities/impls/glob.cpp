//
//   Copyright(c) 2013 szx
//   
//   Permission is hereby granted, free of charge, to any person obtaining a copy
//   of this software and associated documentation files(the "Software"), to deal
//   in the Software without restriction, including without limitation the rights
//   to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
//   copies of the Software, and to permit persons to whom the Software is furnished
//   to do so, subject to the following conditions :
//   
//   The above copyright notice and this permission notice shall be included in all
//   copies or substantial portions of the Software.
//   
//   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
//   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//   THE SOFTWARE.
//
//   https://github.com/sryze/glob
//

#include "../glob.h"
#ifdef _WIN32
#include <windows.h>
#define GLOB_WINDOWS
#else
#include <dirent.h>
#include <fnmatch.h>

#endif

#define GLOB_UNIVERSAL_PATH_SEP "/"
#define GLOB_ALL_PATH_SEP "/\\"
#ifdef GLOB_WINDOWS
#define GLOB_NATIVE_PATH_SEP "\\"
#else
#define GLOB_NATIVE_PATH_SEP GLOB_UNIVERSAL_PATH_SEP
#endif

namespace glob {

#ifdef GLOB_WINDOWS

    class glob_impl
    {
    public:
        glob_impl() :
            find_handle(INVALID_HANDLE_VALUE),
            find_data(),
            has_next(false)
        {
        }

        std::string dir_path;
        HANDLE find_handle;
        WIN32_FIND_DATAA find_data;
        bool has_next;
    };

#else // GLOB_WINDOWS

    class glob_impl
    {
    public:
        glob_impl() :
            dir(nullptr),
            dir_entry(nullptr)
        {
        }

        std::string dir_path;
        std::string file_pattern;
        DIR* dir;
        struct dirent* dir_entry;
    };

#endif // !GLOB_WINDOWS

    namespace {

        std::pair<std::string, std::string> split_path(const std::string& path)
        {
            std::string::size_type last_sep = path.find_last_of(GLOB_ALL_PATH_SEP);
            if (last_sep != std::string::npos) {
                return std::make_pair(
                    std::string(path.begin(), path.begin() + last_sep),
                    std::string(path.begin() + last_sep + 1, path.end()));
            }
            return std::make_pair(".", path);
        }

    } // anonymous namespace

    glob::glob(const std::string& pattern) :
        use_full_paths_(false),
        impl_(std::make_unique<glob_impl>())
    {
        open(pattern);
    }

    glob::~glob()
    {
        close();
    }

    bool glob::use_full_paths() const {
        return use_full_paths_;
    }

    void glob::use_full_paths(bool use_full_paths) {
        use_full_paths_ = use_full_paths;
    }

    void glob::open(const std::string& pattern)
    {
        auto dir_and_file = split_path(pattern);
        impl_->dir_path = dir_and_file.first;

#ifdef GLOB_WINDOWS
        impl_->find_handle =
            FindFirstFileA(pattern.c_str(), &impl_->find_data);
        impl_->has_next = impl_->find_handle != INVALID_HANDLE_VALUE;
#else
        impl_->dir = opendir(impl_->dir_path.c_str());
        impl_->file_pattern = dir_and_file.second;

        if (impl_->dir != nullptr) {
            next();
        }
#endif
    }

    void glob::close()
    {
#ifdef GLOB_WINDOWS
        if (impl_->find_handle != INVALID_HANDLE_VALUE) {
            FindClose(impl_->find_handle);
            impl_->find_handle = INVALID_HANDLE_VALUE;
            impl_->has_next = false;
        }
#else
        if (impl_->dir != nullptr) {
            closedir(impl_->dir);
            impl_->dir = nullptr;
        }
#endif
    }

    std::string glob::current_match() const
    {
        const char* filename;
#ifdef GLOB_WINDOWS
        filename = impl_->find_data.cFileName;
#else
        filename = impl_->dir_entry->d_name;
#endif
        if (use_full_paths_) {
            std::string path(impl_->dir_path);
            path.append(GLOB_NATIVE_PATH_SEP);
            path.append(filename);
            return path;
        }
        else {
            return filename;
        }
    }

    bool glob::is_valid() const
    {
#ifdef GLOB_WINDOWS
        return impl_->has_next;
#else
        return impl_->dir != nullptr && impl_->dir_entry != nullptr;
#endif
    }

    bool glob::next()
    {
#ifdef GLOB_WINDOWS
        impl_->has_next =
            FindNextFileA(impl_->find_handle, &impl_->find_data) != 0;
        return impl_->has_next;
#else
        while ((impl_->dir_entry = readdir(impl_->dir)) != nullptr) {
            if (!fnmatch(impl_->file_pattern.c_str(),
                impl_->dir_entry->d_name,
                FNM_CASEFOLD | FNM_NOESCAPE | FNM_PERIOD)) {
                return true;
            }
        }
        return false;
#endif
    }

} // namespace glob
