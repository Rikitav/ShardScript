#include <ShardScript.hpp>
#include <Windows.h>
#include <cstdio>
#include <stdexcept>
#include <string>

using namespace shard;

namespace shard
{
	extern "C"
	{
		__declspec(dllexport) ObjectInstance* shard_file_ReadAllText(const CallState& context) noexcept(false)
		{
			std::wstring fileName = context.Args[0]->AsString(); // fileName
			std::wifstream fileStream(fileName.c_str());

			if (!fileStream.is_open())
				throw std::runtime_error("Failed to open text file.");

			std::wstring content = std::wstring(std::istreambuf_iterator<wchar_t>(fileStream), std::istreambuf_iterator<wchar_t>());
			return context.Collector.FromValue(content);
		}

		__declspec(dllexport) ObjectInstance* shard_file_WriteAllText(const CallState& context) noexcept(false)
		{
			std::wstring fileName = context.Args[0]->AsString(); // fileName
			std::wstring content = context.Args[1]->AsString(); // content
			std::wofstream fileStream(fileName.c_str());

			if (!fileStream.is_open())
				throw std::runtime_error("Failed to open text file.");

			fileStream.write(content.c_str(), content.size());

			if (fileStream.fail())
				throw std::runtime_error("File writing failed.");

			return nullptr;
		}

		__declspec(dllexport) ObjectInstance* shard_file_AppendAllText(const CallState& context) noexcept(false)
		{
			std::wstring fileName = context.Args[0]->AsString(); // fileName
			std::wstring content = context.Args[1]->AsString(); // content
			std::wofstream fileStream(fileName.c_str(), std::ios_base::app);

			if (!fileStream.is_open())
				throw std::runtime_error("Failed to open text file.");
			
			fileStream << content;
			return nullptr;
		}

		__declspec(dllexport) ObjectInstance* shard_file_Exists(const CallState& context) noexcept(false)
		{
			std::wstring fileName = context.Args[0]->AsString();
			DWORD attrib = GetFileAttributesW(fileName.c_str());
			bool exists = (attrib != INVALID_FILE_ATTRIBUTES && !(attrib & FILE_ATTRIBUTE_DIRECTORY));
			return context.Collector.FromValue(exists);
		}

		__declspec(dllexport) ObjectInstance* shard_file_Delete(const CallState& context) noexcept(false)
		{
			std::wstring fileName = context.Args[0]->AsString();
			if (_wremove(fileName.c_str()) != 0)
				throw std::runtime_error("Failed to delete file.");

			return nullptr;
		}

		__declspec(dllexport) ObjectInstance* shard_file_Copy(const CallState& context) noexcept(false)
		{
			std::wstring sourceFileName = context.Args[0]->AsString();
			std::wstring destFileName = context.Args[1]->AsString();
			if (!CopyFileW(sourceFileName.c_str(), destFileName.c_str(), FALSE))
				throw std::runtime_error("Failed to copy file.");

			return nullptr;
		}

		__declspec(dllexport) ObjectInstance* shard_file_Move(const CallState& context) noexcept(false)
		{
			std::wstring sourceFileName = context.Args[0]->AsString();
			std::wstring destFileName = context.Args[1]->AsString();
			if (!MoveFileW(sourceFileName.c_str(), destFileName.c_str()))
				throw std::runtime_error("Failed to move file.");

			return nullptr;
		}
	}
}
