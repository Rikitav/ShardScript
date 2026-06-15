#include <ShardScript.hpp>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

namespace fs = std::filesystem;
using namespace shard;

namespace shard
{
	extern "C"
	{
		SHARDLIB_EXPORT ObjectInstance* shard_file_ReadAllText(const CallState& context) noexcept(false)
		{
			std::wstring fileName = context.Args[0]->AsString(); // fileName
			std::wifstream fileStream = std::wifstream(fs::path(fileName));

			if (!fileStream.is_open())
				throw std::runtime_error("Failed to open text file.");

			std::wstring content = std::wstring(std::istreambuf_iterator<wchar_t>(fileStream), std::istreambuf_iterator<wchar_t>());
			return context.Collector.FromValue(content);
		}

		SHARDLIB_EXPORT ObjectInstance* shard_file_WriteAllText(const CallState& context) noexcept(false)
		{
			std::wstring fileName = context.Args[0]->AsString(); // fileName
			std::wstring content = context.Args[1]->AsString(); // content
			std::wofstream fileStream = std::wofstream(fs::path(fileName));

			if (!fileStream.is_open())
				throw std::runtime_error("Failed to open text file.");

			fileStream.write(content.c_str(), content.size());
			if (fileStream.fail())
				throw std::runtime_error("File writing failed.");

			return nullptr;
		}

		SHARDLIB_EXPORT ObjectInstance* shard_file_AppendAllText(const CallState& context) noexcept(false)
		{
			std::wstring fileName = context.Args[0]->AsString(); // fileName
			std::wstring content = context.Args[1]->AsString(); // content
			std::wofstream fileStream = std::wofstream(fs::path(fileName), std::ios_base::app);

			if (!fileStream.is_open())
				throw std::runtime_error("Failed to open text file.");
			
			fileStream << content;
			return nullptr;
		}

		SHARDLIB_EXPORT ObjectInstance* shard_file_Exists(const CallState& context) noexcept(false)
		{
			std::wstring fileName = context.Args[0]->AsString();
			bool exists = fs::exists(fileName);
			return context.Collector.FromValue(exists);
		}

		SHARDLIB_EXPORT ObjectInstance* shard_file_Delete(const CallState& context) noexcept(false)
		{
			std::wstring fileName = context.Args[0]->AsString();
			if (!fs::remove(fileName))
				throw std::runtime_error("Failed to delete file.");

			return nullptr;
		}

		SHARDLIB_EXPORT ObjectInstance* shard_file_Copy(const CallState& context) noexcept(false)
		{
			std::wstring sourceFileName = context.Args[0]->AsString();
			std::wstring destFileName = context.Args[1]->AsString();

			if (!fs::copy_file(sourceFileName.c_str(), destFileName.c_str(), std::filesystem::copy_options::overwrite_existing))
				throw std::runtime_error("Failed to copy file.");

			return nullptr;
		}

		SHARDLIB_EXPORT ObjectInstance* shard_file_Move(const CallState& context) noexcept(false)
		{
			std::wstring sourceFileName = context.Args[0]->AsString();
			std::wstring destFileName = context.Args[1]->AsString();

			if (!fs::copy_file(sourceFileName.c_str(), destFileName.c_str(), std::filesystem::copy_options::overwrite_existing))
				throw std::runtime_error("Failed to move file.");

			if (!fs::remove(sourceFileName.c_str()))
				throw std::runtime_error("Failed to move file.");

			return nullptr;
		}
	}
}
