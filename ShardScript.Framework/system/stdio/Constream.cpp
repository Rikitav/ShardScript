#include <ShardScript.hpp>
#include <stdexcept>
#include <string>
#include <random>
#include <climits>
#include <cstdint>
#include <string>
#include <iostream>

using namespace shard;

namespace shard
{
	extern "C"
	{
		__declspec(dllexport) ObjectInstance* shard_constream_print(const CallState& context) noexcept(false)
		{
			ObjectInstance* instance = context.Args[0]; // var
			TypeSymbol* type = const_cast<TypeSymbol*>(instance->getInfo());

			if (type->IsPrimitive())
			{
				ConsoleHelper::Write(instance);
				return nullptr; // void
			}

			static std::wstring methodWName = L"ToString";
			MethodSymbol* toString = type->FindMethod(methodWName, std::vector<TypeSymbol*>());

			if (toString != nullptr)
			{
				context.Runtimer.InvokeMethod(toString, { instance });
				ObjectInstance* result = context.Runtimer.CurrentFrame()->PopStack();

				if (type != SymbolTable::Primitives::String)
				{
#pragma warning (push)
#pragma warning (disable: 4244)
					std::string methodName = std::string(toString->FullName.begin(), toString->FullName.end());
					throw std::runtime_error("Failed to evaluate ToString method of \'" + methodName + "\'. Reason: returned not a string!");
#pragma warning (pop)
				}

				ConsoleHelper::Write(instance);
				return nullptr; // void
			}

			ConsoleHelper::Write(type->FullName);
			return nullptr; // void
		}

		__declspec(dllexport) ObjectInstance* shard_constream_println(const CallState& context) noexcept(false)
		{
			ObjectInstance* instance = context.Args[0]; // var
			TypeSymbol* type = const_cast<TypeSymbol*>(instance->getInfo());

			if (type->IsPrimitive())
			{
				ConsoleHelper::WriteLine(instance);
				return nullptr; // void
			}

			static std::wstring methodWName = L"ToString";
			MethodSymbol* toString = type->FindMethod(methodWName, std::vector<TypeSymbol*>());

			if (toString != nullptr)
			{
				context.Runtimer.InvokeMethod(toString, { instance });
				ObjectInstance* result = context.Runtimer.CurrentFrame()->PopStack();

				if (result->getInfo() != SymbolTable::Primitives::String)
				{
					std::string methodName = std::string(toString->FullName.begin(), toString->FullName.end());
					throw std::runtime_error("Failed to evaluate ToString method of \'" + methodName + "\'. Reason: returned not a string!");
				}

				ConsoleHelper::WriteLine(result);
				return nullptr; // void
			}

			ConsoleHelper::WriteLine(type->FullName);
			return nullptr; // void
		}

		__declspec(dllexport) ObjectInstance* shard_constream_input(const CallState& context) noexcept(false)
		{
			std::wstring input;
			getline(std::wcin, input);
			return context.Collector.FromValue(input);
		}
	}
}
