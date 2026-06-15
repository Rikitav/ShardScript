#include <ShardScript.hpp>
#include <Windows.h>
#include <string>
#include <stdexcept>

using namespace shard;

namespace shard
{
	extern "C"
	{
		__declspec(dllexport) ObjectInstance* shard_directory_GetDirectory(const CallState& context) noexcept(false)
		{
			ObjectInstance* fullName = context.Args[0];
			ObjectInstance* instance = context.Collector.AllocateInstance(context.Method->ReturnType);

			TypeSymbol* ownerType = const_cast<TypeSymbol*>(instance->getInfo());
			FieldSymbol* field = ownerType->Fields[0];

			instance->SetField(field, fullName);
			return instance;
		}

		__declspec(dllexport) ObjectInstance* shard_directory_Name_get(const CallState& context) noexcept(false)
		{
			ObjectInstance* instance = context.Args[0];
			TypeSymbol* ownerType = const_cast<TypeSymbol*>(instance->getInfo());

			FieldSymbol* field = ownerType->Fields[0];
			ObjectInstance* fullName = instance->GetField(field);
			std::wstring path = fullName->AsString();

			std::size_t pos = path.find_last_of(L"\\/");
			if (pos != std::wstring::npos && pos != path.length() - 1)
				path = path.substr(pos + 1);

			return context.Collector.FromValue(path);
		}

		__declspec(dllexport) ObjectInstance* shard_directory_Exists_get(const CallState& context) noexcept(false)
		{
			ObjectInstance* instance = context.Args[0];
			TypeSymbol* ownerType = const_cast<TypeSymbol*>(instance->getInfo());

			FieldSymbol* field = ownerType->Fields[0];
			ObjectInstance* fullName = instance->GetField(field);

			DWORD attrib = GetFileAttributesW(fullName->AsString());
			bool exists = (attrib != INVALID_FILE_ATTRIBUTES && (attrib & FILE_ATTRIBUTE_DIRECTORY));
			return context.Collector.FromValue(exists);
		}

		__declspec(dllexport) ObjectInstance* shard_directory_Create(const CallState& context) noexcept(false)
		{
			ObjectInstance* instance = context.Args[0];
			TypeSymbol* ownerType = const_cast<TypeSymbol*>(instance->getInfo());

			FieldSymbol* field = ownerType->Fields[0];
			ObjectInstance* fullName = instance->GetField(field);

			if (!CreateDirectoryW(fullName->AsString(), nullptr))
			{
				if (GetLastError() != ERROR_ALREADY_EXISTS)
					throw std::runtime_error("Failed to create directory.");
			}

			return nullptr;
		}

		__declspec(dllexport) ObjectInstance* shard_directory_Delete(const CallState& context) noexcept(false)
		{
			ObjectInstance* instance = context.Args[0];
			TypeSymbol* ownerType = const_cast<TypeSymbol*>(instance->getInfo());

			FieldSymbol* field = ownerType->Fields[0];
			ObjectInstance* fullName = instance->GetField(field);

			if (!RemoveDirectoryW(fullName->AsString()))
				throw std::runtime_error("Failed to delete directory.");

			return nullptr;
		}

		__declspec(dllexport) ObjectInstance* shard_directory_Exists(const CallState& context) noexcept(false)
		{
			std::wstring path = context.Args[0]->AsString();
			DWORD attrib = GetFileAttributesW(path.c_str());
			bool exists = (attrib != INVALID_FILE_ATTRIBUTES && (attrib & FILE_ATTRIBUTE_DIRECTORY));
			return context.Collector.FromValue(exists);
		}

		__declspec(dllexport) ObjectInstance* shard_directory_CreateDirectory(const CallState& context) noexcept(false)
		{
			std::wstring path = context.Args[0]->AsString();
			if (!CreateDirectoryW(path.c_str(), nullptr))
			{
				if (GetLastError() != ERROR_ALREADY_EXISTS)
					throw std::runtime_error("Failed to create directory.");
			}

			ObjectInstance* instance = context.Collector.AllocateInstance(context.Method->ReturnType);
			TypeSymbol* ownerType = const_cast<TypeSymbol*>(instance->getInfo());
			FieldSymbol* field = ownerType->Fields[0];
			instance->SetField(field, context.Args[0]);
			return instance;
		}

		__declspec(dllexport) ObjectInstance* shard_directory_DeleteStatic(const CallState& context) noexcept(false)
		{
			std::wstring path = context.Args[0]->AsString();
			if (!RemoveDirectoryW(path.c_str()))
				throw std::runtime_error("Failed to delete directory.");

			return nullptr;
		}
	}
}
