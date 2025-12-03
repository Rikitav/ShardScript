#include <shard/framework/FrameworkModule.h>

#include <shard/runtime/InboundVariablesContext.h>
#include <shard/runtime/GarbageCollector.h>
#include <shard/runtime/ObjectInstance.h>

#include <shard/syntax/symbols/MethodSymbol.h>
#include <shard/syntax/symbols/FieldSymbol.h>
#include <shard/syntax/symbols/ClassSymbol.h>

#include <string>

using namespace shard::runtime;
using namespace shard::syntax::symbols;

namespace shard::framework
{
	class FileSystem_Directory : public FrameworkModule
	{
		static ObjectInstance* Impl_GetDirectory(MethodSymbol* symbol, InboundVariablesContext* arguments)
		{
			static FieldSymbol* field = static_cast<ClassSymbol*>(symbol->Parent)->Fields.at(0);

			ObjectInstance* fullName = arguments->Variables.at(L"fullName");
			ObjectInstance* instance = GarbageCollector::AllocateInstance(symbol->ReturnType);

			instance->SetField(field, fullName);
			return instance;
		}

	public:
		std::wstring& FrameworkModule::GetSourceCode()
		{
			static std::wstring SourceCode =
			L"																				  \
			namespace System.IO																  \
			{																				  \
				public class DirectoryInfo											          \
				{																			  \
					public string FullName { get; private set; }							  \
																							  \
					public static extern DirectoryInfo GetDirectory(string fullName);	      \
				}																			  \
			}																				  \
			";

			return SourceCode;
		}

		bool FrameworkModule::BindMethod(MethodSymbol* symbol)
		{
			if (symbol->Name == L"GetDirectory")
			{
				symbol->HandleType = MethodHandleType::External;
				symbol->FunctionPointer = Impl_GetDirectory;
				return true;
			}

			return false;
		}
	};
}
