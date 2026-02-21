#include <shard/runtime/framework/FrameworkModule.hpp>

#include <shard/runtime/ArgumentsSpan.hpp>
#include <shard/runtime/GarbageCollector.hpp>
#include <shard/runtime/ObjectInstance.hpp>
#include <shard/runtime/VirtualMachine.hpp>

#include <shard/syntax/symbols/MethodSymbol.hpp>
#include <shard/syntax/symbols/FieldSymbol.hpp>
#include <shard/syntax/symbols/ClassSymbol.hpp>
#include <shard/syntax/symbols/AccessorSymbol.hpp>
#include <shard/syntax/symbols/ConstructorSymbol.hpp>

#include <shard/parsing/lexical/SourceProvider.hpp>
#include <shard/parsing/lexical/LexicalAnalyzer.hpp>
#include <shard/parsing/lexical/reading/StringStreamReader.hpp>

#include <string>

#include "../../resources.hpp"

using namespace shard;

namespace shard
{
	class FileSystem_Directory : public FrameworkModule
	{
		static ObjectInstance* Impl_GetDirectory(const VirtualMachine* host, const MethodSymbol* method, ArgumentsSpan& arguments)
		{
			static FieldSymbol* field = static_cast<ClassSymbol*>(method->Parent)->Fields.at(0);

			ObjectInstance* fullName = arguments[0]; // fullName
			ObjectInstance* instance = GarbageCollector::AllocateInstance(method->ReturnType);

			instance->SetField(field, fullName);
			return instance;
		}

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
				symbol->FunctionPointer = Impl_GetDirectory;
				return true;
			}

			return false;
		}

		bool FrameworkModule::BindAccessor(AccessorSymbol* symbol)
		{
			return false;
		}
	};
}
