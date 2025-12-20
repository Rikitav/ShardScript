#include <shard/framework/FrameworkModule.h>

#include <shard/runtime/InboundVariablesContext.h>
#include <shard/runtime/ObjectInstance.h>
#include <shard/syntax/symbols/MethodSymbol.h>

#include <shard/parsing/reading/SourceReader.h>
#include <shard/parsing/reading/StringStreamReader.h>
#include <shard/parsing/semantic/SymbolTable.h>

#include <Windows.h>
#include <iostream>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <random>

#include "../resources.h"

using namespace shard::runtime;
using namespace shard::parsing;
using namespace shard::syntax::symbols;

namespace shard::framework
{
	class Random : public FrameworkModule
	{
		static ObjectInstance* Impl_Integer(const MethodSymbol* method, InboundVariablesContext* arguments)
		{
			std::random_device rd;
			std::mt19937 gen(rd()); // (std::chrono::steady_clock::now().time_since_epoch().count());
			std::uniform_int_distribution<> distrib(LONG_MIN, LONG_MAX);

			long value = distrib(gen);
			return ObjectInstance::FromValue(value);
        }

		static ObjectInstance* Impl_Integer_Top(const MethodSymbol* method, InboundVariablesContext* arguments)
		{
			ObjectInstance* arg_top = arguments->Variables.at(L"top");
			int top = arg_top->AsInteger();

			if (top == 0)
				return ObjectInstance::FromValue(0.0);

			if (top < 0)
				throw std::runtime_error("top value must be greater than zero");

			std::random_device rd;
			std::mt19937 gen(rd());
			std::uniform_int_distribution<> distrib(0, INT_MAX);

			long value = distrib(gen);
			return ObjectInstance::FromValue(value);
		}

		static ObjectInstance* Impl_Integer_Bottom_Top(const MethodSymbol* method, InboundVariablesContext* arguments)
		{
			ObjectInstance* arg_bottom = arguments->Variables.at(L"bottom");
			ObjectInstance* arg_top = arguments->Variables.at(L"top");

			long bottom = arg_bottom->AsInteger();
			long top = arg_top->AsInteger();

			if (bottom == top)
				return ObjectInstance::FromValue(bottom);

			if (top < bottom)
				throw std::runtime_error("top edge value must be greater than bottom edge value");

			std::random_device rd;
			std::mt19937 gen(rd());
			std::uniform_int_distribution<> distrib(bottom, top);

			long value = distrib(gen);
			return ObjectInstance::FromValue(value);
		}

		static ObjectInstance* Impl_Double(const MethodSymbol* method, InboundVariablesContext* arguments)
		{
			std::random_device rd;
			std::mt19937 gen(rd());
			std::uniform_real_distribution<> real_distrib(0.0, 1.0);

			double value = real_distrib(gen);
			return ObjectInstance::FromValue(value);
		}

		static ObjectInstance* Impl_Double_Top(const MethodSymbol* method, InboundVariablesContext* arguments)
		{
			ObjectInstance* arg_top = arguments->Variables.at(L"top");
			double top = arg_top->AsDouble();

			if (top == 0)
				return ObjectInstance::FromValue(0.0);

			if (top < 0)
				throw std::runtime_error("top value must be greater than zero");

			std::random_device rd;
			std::mt19937 gen(rd());
			std::uniform_real_distribution<> real_distrib(0.0, top);

			double value = real_distrib(gen);
			return ObjectInstance::FromValue(value);
		}

		static ObjectInstance* Impl_Double_Bottom_Top(const MethodSymbol* method, InboundVariablesContext* arguments)
		{
			ObjectInstance* arg_bottom = arguments->Variables.at(L"bottom");
			ObjectInstance* arg_top = arguments->Variables.at(L"top");

			double bottom = arg_bottom->AsDouble();
			double top = arg_top->AsDouble();

			if (bottom == top)
				return ObjectInstance::FromValue(bottom);

			if (top < bottom)
				throw std::runtime_error("top edge value must be greater than bottom edge value");

			std::random_device rd;
			std::mt19937 gen(rd());
			std::uniform_real_distribution<> real_distrib(bottom, top);

			double value = real_distrib(gen);
			return ObjectInstance::FromValue(value);
		}

		static ObjectInstance* Impl_Propably(const MethodSymbol* method, InboundVariablesContext* arguments)
		{
			ObjectInstance* arg_chance = arguments->Variables.at(L"chance");
			double chance = arg_chance->AsInteger();

			if (chance == 0)
				return ObjectInstance::FromValue(false);

			if (chance == 100)
				return ObjectInstance::FromValue(true);

			if (chance < 0 || chance > 100)
				throw std::runtime_error("chance must be greater than 0 and less than 100");

			std::random_device rd;
			std::mt19937 gen(rd());
			std::bernoulli_distribution bernoulli_dist(chance / 100);

			bool value = bernoulli_dist(gen);
			return ObjectInstance::FromValue(value);
		}

	public:

		SourceReader* FrameworkModule::GetSource()
		{
			const wchar_t* resourceData; size_t resourceSize;
			resources::GetResource(L"SYSTEM_RANDOM", resourceData, resourceSize);
			return new StringStreamReader(L"Random.ss", resourceData, resourceSize / sizeof(wchar_t));
		}

		bool FrameworkModule::BindConstructor(ConstructorSymbol * symbol)
		{
			return false;
		}

		bool FrameworkModule::BindAccessor(AccessorSymbol* symbol)
		{
			return false;
		}

		bool FrameworkModule::BindMethod(MethodSymbol * symbol)
		{
			if (symbol->Name == L"Integer")
			{
				switch (symbol->Parameters.size())
				{
					case 0:
					{
						symbol->FunctionPointer = Impl_Integer;
						return true;
					}

					case 1:
					{
						symbol->FunctionPointer = Impl_Integer_Top;
						return true;
					}

					case 2:
					{
						symbol->FunctionPointer = Impl_Integer_Bottom_Top;
						return true;
					}

					default:
						return false;
				}
			}

			if (symbol->Name == L"Double")
			{
				switch (symbol->Parameters.size())
				{
					case 0:
					{
						symbol->FunctionPointer = Impl_Double;
						return true;
					}

					case 1:
					{
						symbol->FunctionPointer = Impl_Double_Top;
						return true;
					}

					case 2:
					{
						symbol->FunctionPointer = Impl_Double_Bottom_Top;
						return true;
					}

					default:
						return false;
				}
			}

			if (symbol->Name == L"Propably")
			{
				symbol->FunctionPointer = Impl_Propably;
				return true;
			}

			return false;
		}
	};
}