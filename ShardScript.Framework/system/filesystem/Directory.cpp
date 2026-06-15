#include <string>
#include <stdexcept>
#include <filesystem>
#include <ShardScript.hpp>

namespace fs = std::filesystem;
using namespace shard;

namespace shard
{
	extern "C"
	{
		SHARDLIB_EXPORT ObjectInstance* shard_directory_GetDirectory(const CallState& context) noexcept(false)
		{
			ObjectInstance* fullName = context.Args[0];
			ObjectInstance* instance = context.Collector.AllocateInstance(context.Method->ReturnType);

			TypeSymbol* ownerType = const_cast<TypeSymbol*>(instance->getInfo());
			FieldSymbol* field = ownerType->Fields[0];

			instance->SetField(field, fullName);
			return instance;
		}

		SHARDLIB_EXPORT ObjectInstance* shard_directory_Name_get(const CallState& context) noexcept(false)
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

		SHARDLIB_EXPORT ObjectInstance* shard_directory_Exists_get(const CallState& context) noexcept(false)
		{
			ObjectInstance* instance = context.Args[0];
			TypeSymbol* ownerType = const_cast<TypeSymbol*>(instance->getInfo());

			FieldSymbol* field = ownerType->Fields[0];
			ObjectInstance* fullName = instance->GetField(field);

			bool exists = fs::exists(fullName->AsString());
			return context.Collector.FromValue(exists);
		}

		SHARDLIB_EXPORT ObjectInstance* shard_directory_Create(const CallState& context) noexcept(false)
		{
			ObjectInstance* instance = context.Args[0];
			TypeSymbol* ownerType = const_cast<TypeSymbol*>(instance->getInfo());

			FieldSymbol* field = ownerType->Fields[0];
			ObjectInstance* fullName = instance->GetField(field);

			if (fs::exists(fullName->AsString()))
				return nullptr;

			if (!fs::create_directories(fullName->AsString()))
				throw std::runtime_error("Failed to create directory.");

			return nullptr;
		}

		SHARDLIB_EXPORT ObjectInstance* shard_directory_Delete(const CallState& context) noexcept(false)
		{
			ObjectInstance* instance = context.Args[0];
			TypeSymbol* ownerType = const_cast<TypeSymbol*>(instance->getInfo());

			FieldSymbol* field = ownerType->Fields[0];
			ObjectInstance* fullName = instance->GetField(field);

			if (!fs::remove(fullName->AsString()))
				throw std::runtime_error("Failed to delete directory.");

			return nullptr;
		}

		SHARDLIB_EXPORT ObjectInstance* shard_directory_Exists(const CallState& context) noexcept(false)
		{
			std::wstring path = context.Args[0]->AsString();
			bool exists = fs::exists(path);;
			return context.Collector.FromValue(exists);
		}

		SHARDLIB_EXPORT ObjectInstance* shard_directory_CreateDirectory(const CallState& context) noexcept(false)
		{
			std::wstring path = context.Args[0]->AsString();
			if (fs::exists(path))
				return nullptr;

			if (!fs::create_directories(path))
				throw std::runtime_error("Failed to create directory.");

			ObjectInstance* instance = context.Collector.AllocateInstance(context.Method->ReturnType);
			TypeSymbol* ownerType = const_cast<TypeSymbol*>(instance->getInfo());
			FieldSymbol* field = ownerType->Fields[0];
			instance->SetField(field, context.Args[0]);
			return instance;
		}

		SHARDLIB_EXPORT ObjectInstance* shard_directory_DeleteStatic(const CallState& context) noexcept(false)
		{
			std::wstring path = context.Args[0]->AsString();
			if (!fs::remove(path))
				throw std::runtime_error("Failed to delete directory.");

			return nullptr;
		}
	}
}
