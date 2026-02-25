#include <string>

#include <ShardScript.hpp>
#include <resources.hpp>

using namespace shard;

namespace shard
{
	class FileSystem_Directory : public FrameworkModule
	{
		// TODO: fix
		/*
		static ObjectInstance* Impl_GetDirectory(const VirtualMachine* host, const MethodSymbol* method, ArgumentsSpan& arguments)
		{
			static FieldSymbol* field = static_cast<ClassSymbol*>(method->Parent)->Fields.at(0);

			ObjectInstance* fullName = arguments[0]; // fullName
			ObjectInstance* instance = GarbageCollector::AllocateInstance(method->ReturnType);

			instance->SetField(field, fullName);
			return instance;
		}
		*/

	public:
		SourceProvider* FrameworkModule::GetSource()
		{
			const wchar_t* resourceData; size_t resourceSize;
			resources::GetResource(L"FILESYSTEM_DIRECTORY", resourceData, resourceSize);
			StringStreamReader* reader = new StringStreamReader(L"Directory.ss", resourceData, resourceSize / sizeof(wchar_t));
			return new LexicalAnalyzer(reader, true);
		}

		bool FrameworkModule::BindConstructor(ConstructorSymbol* symbol)
		{
			return false;
		}

		bool FrameworkModule::BindMethod(MethodSymbol* symbol)
		{
			if (symbol->Name == L"GetDirectory")
			{
				/*
				symbol->FunctionPointer = Impl_GetDirectory;
				return true;
				*/
			}

			return false;
		}

		bool FrameworkModule::BindAccessor(AccessorSymbol* symbol)
		{
			return false;
		}
	};
}
