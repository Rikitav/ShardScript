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
		static ObjectInstance* Impl_ReadAllText(const MethodSymbol* symbol, InboundVariablesContext* arguments)
		{
			std::wstring fileName = arguments->Variables.at(L"fileName")->ReadPrimitive<std::wstring>();
			std::wifstream fileStream(fileName);

			if (!fileStream.is_open())
				throw std::runtime_error("Failed to open text file.");

			std::wstring content = std::wstring(std::istreambuf_iterator<wchar_t>(fileStream), std::istreambuf_iterator<wchar_t>());
			return ObjectInstance::FromValue(content);
		}

		static ObjectInstance* Impl_WriteAllText(const MethodSymbol* symbol, InboundVariablesContext* arguments)
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
			static std::wstring SourceCode =
			L"																				     \
			namespace System.IO																     \
			{																				     \
				public static class File													     \
				{																			     \
					public static extern string ReadAllText(string fileName);				     \
					public static extern string WriteAllText(string fileName, string content);	 \
				}																			     \
			}																				     \
			";

			return SourceCode;
		}

		bool FrameworkModule::BindMethod(MethodSymbol* symbol)
		{
			if (symbol->Name == L"ReadAllText")
			{
				symbol->HandleType = MethodHandleType::External;
				symbol->FunctionPointer = Impl_ReadAllText;
				return true;
			}

			if (symbol->Name == L"WriteAllText")
			{
				symbol->HandleType = MethodHandleType::External;
				symbol->FunctionPointer = Impl_WriteAllText;
				return true;
			}

			return false;
		}
	};
}