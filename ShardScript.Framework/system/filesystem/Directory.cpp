#include <shard/runtime/framework/FrameworkModule.h>

#include <shard/runtime/InboundVariablesContext.h>
#include <shard/runtime/GarbageCollector.h>
#include <shard/runtime/ObjectInstance.h>

#include <shard/syntax/symbols/MethodSymbol.h>
#include <shard/syntax/symbols/FieldSymbol.h>
#include <shard/syntax/symbols/ClassSymbol.h>

#include <shard/parsing/lexical/SourceProvider.h>
#include <shard/parsing/lexical/LexicalAnalyzer.h>
#include <shard/parsing/lexical/reading/StringStreamReader.h>
#include <shard/parsing/semantic/SymbolTable.h>

#include <Windows.h>
#include <string>

#include "../resources.h"

using namespace shard;

namespace shard
{
	class FileSystem_Directory : public FrameworkModule
	{
		static ObjectInstance* Impl_GetDirectory(const MethodSymbol* symbol, InboundVariablesContext* arguments)
		{
			static FieldSymbol* field = static_cast<ClassSymbol*>(symbol->Parent)->Fields.at(0);

			ObjectInstance* fullName = arguments->Variables.at(L"fullName");
			ObjectInstance* instance = GarbageCollector::AllocateInstance(symbol->ReturnType);

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
