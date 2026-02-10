#include <shard/runtime/framework/FrameworkModule.h>

#include <shard/runtime/ArgumentsSpan.h>
#include <shard/runtime/GarbageCollector.h>
#include <shard/runtime/ObjectInstance.h>
#include <shard/runtime/VirtualMachine.h>

#include <shard/syntax/symbols/MethodSymbol.h>
#include <shard/syntax/symbols/FieldSymbol.h>
#include <shard/syntax/symbols/ClassSymbol.h>
#include <shard/syntax/symbols/AccessorSymbol.h>
#include <shard/syntax/symbols/ConstructorSymbol.h>

#include <shard/parsing/lexical/SourceProvider.h>
#include <shard/parsing/lexical/LexicalAnalyzer.h>
#include <shard/parsing/lexical/reading/StringStreamReader.h>

#include <string>

#include "../../resources.h"

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
