#include <shard/framework/FrameworkModule.h>

#include <shard/runtime/InboundVariablesContext.h>
#include <shard/runtime/ObjectInstance.h>
#include <shard/syntax/symbols/MethodSymbol.h>

#include <iostream>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>

using namespace shard::runtime;
using namespace shard::syntax::symbols;

namespace shard::framework
{
	class FileSystem_File : public FrameworkModule
	{
		std::wstring SourceCode =
		L"																				  \
		namespace System																  \
		{																				  \
			public static class File													  \
			{																			  \
				public static string ReadAllText(string fileName);						  \
				public static string WriteAllText(string fileName, string content);		  \
		   }																			  \
		}																				  \
		";

		static ObjectInstance* Impl_ReadAllText(InboundVariablesContext* arguments)
		{
			std::wstring fileName = arguments->Variables.at(L"fileName")->ReadPrimitive<std::wstring>();
			std::wifstream fileStream(fileName);

			if (!fileStream.is_open())
				throw std::runtime_error("Failed to open text file.");

			std::wstring content = std::wstring(std::istreambuf_iterator<wchar_t>(fileStream), std::istreambuf_iterator<wchar_t>());
			return ObjectInstance::FromValue(content);
		}

		static ObjectInstance* Impl_WriteAllText(InboundVariablesContext* arguments)
		{
			std::wstring fileName = arguments->Variables.at(L"fileName")->ReadPrimitive<std::wstring>();
			std::wstring content = arguments->Variables.at(L"content")->ReadPrimitive<std::wstring>();
			std::wofstream fileStream(fileName);

			if (!fileStream.is_open())
				throw std::runtime_error("Failed to open text file.");

			fileStream.write(content.c_str(), content.size());

			if (fileStream.fail())
				throw std::runtime_error("File writing failed.");

			return nullptr;
		}

	public:
		std::wstring& FrameworkModule::GetSourceCode()
		{
			return SourceCode;
		}

		bool FrameworkModule::BindMethod(MethodSymbol* symbol)
		{
			if (symbol->Name == L"ReadAllText")
			{
				symbol->HandleType = MethodHandleType::FunctionPointer;
				symbol->FunctionPointer = Impl_ReadAllText;
				return true;
			}

			if (symbol->Name == L"WriteAllText")
			{
				symbol->HandleType = MethodHandleType::FunctionPointer;
				symbol->FunctionPointer = Impl_WriteAllText;
				return true;
			}

			return false;
		}
	};
}