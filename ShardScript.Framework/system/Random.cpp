#include <ShardScript.hpp>
#include <stdexcept>
#include <string>
#include <random>
#include <climits>
#include <cstdint>

#include "../resources.hpp"

using namespace shard;

namespace shard
{
	class Random : public FrameworkModule
	{
		static ObjectInstance* Impl_Integer(const CallState& context)
		{
			std::random_device rd;
			std::mt19937 gen(rd()); // (std::chrono::steady_clock::now().time_since_epoch().count());
			std::uniform_int_distribution<> distrib(LONG_MIN, LONG_MAX);

			int64_t value = distrib(gen);
			return context.Domain.GetGarbageCollector().FromValue(value);
        }

		static ObjectInstance* Impl_Integer_Top(const CallState& context)
		{
			int64_t top = context.Args[0]->AsInteger(); // top

			if (top == 0)
				return context.Collector.FromValue(0.0);

			if (top < 0)
				throw std::runtime_error("top value must be greater than zero");

			std::random_device rd;
			std::mt19937 gen(rd());
			std::uniform_int_distribution<> distrib(0, INT_MAX);

			int64_t value = distrib(gen);
			return context.Collector.FromValue(value);
		}

		static ObjectInstance* Impl_Integer_Bottom_Top(const CallState& context)
		{
			int64_t bottom = context.Args[0]->AsInteger(); // bottom
			int64_t top = context.Args[1]->AsInteger(); // top

			if (bottom == top)
				return context.Collector.FromValue(bottom);

			if (top < bottom)
				throw std::runtime_error("top edge value must be greater than bottom edge value");

			std::random_device rd;
			std::mt19937 gen(rd());
			std::uniform_int_distribution<> distrib(static_cast<int>(bottom), static_cast<int>(top));

			int64_t value = distrib(gen);
			return context.Collector.FromValue(value);
		}

		static ObjectInstance* Impl_Double(const CallState& context)
		{
			std::random_device rd;
			std::mt19937 gen(rd());
			std::uniform_real_distribution<> real_distrib(0.0, 1.0);

			double value = real_distrib(gen);
			return context.Collector.FromValue(value);
		}

		static ObjectInstance* Impl_Double_Top(const CallState& context)
		{
			double top = context.Args[0]->AsDouble(); // top

			if (top == 0)
				return context.Collector.FromValue(0.0);

			if (top < 0)
				throw std::runtime_error("top value must be greater than zero");

			std::random_device rd;
			std::mt19937 gen(rd());
			std::uniform_real_distribution<> real_distrib(0.0, top);

			double value = real_distrib(gen);
			return context.Collector.FromValue(value);
		}

		static ObjectInstance* Impl_Double_Bottom_Top(const CallState& context)
		{
			double bottom = context.Args[0]->AsDouble(); // bottom
			double top = context.Args[1]->AsDouble(); // top

			if (bottom == top)
				return context.Collector.FromValue(bottom);

			if (top < bottom)
				throw std::runtime_error("top edge value must be greater than bottom edge value");

			std::random_device rd;
			std::mt19937 gen(rd());
			std::uniform_real_distribution<> real_distrib(bottom, top);

			double value = real_distrib(gen);
			return context.Collector.FromValue(value);
		}

		static ObjectInstance* Impl_Propably(const CallState& context)
		{
			double chance = context.Args[0]->AsDouble(); // chance

			if (chance == 0)
				return context.Collector.FromValue(false);

			if (chance == 100)
				return context.Collector.FromValue(true);

			if (chance < 0 || chance > 100)
				throw std::runtime_error("chance must be greater than 0 and less than 100");

			std::random_device rd;
			std::mt19937 gen(rd());
			std::bernoulli_distribution bernoulli_dist(chance / 100);

			bool value = bernoulli_dist(gen);
			return context.Collector.FromValue(value);
		}

	public:
		SourceProvider* FrameworkModule::GetSource()
		{
			const wchar_t* resourceData; size_t resourceSize;
			resources::GetResource(L"SYSTEM_RANDOM", resourceData, resourceSize);
			StringStreamReader* reader = new StringStreamReader(L"Random.ss", resourceData, resourceSize / sizeof(wchar_t));
			return new LexicalAnalyzer(reader, true);
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