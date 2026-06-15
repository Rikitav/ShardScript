#include <stdexcept>
#include <random>
#include <climits>
#include <cstdint>
#include <ShardScript.hpp>

using namespace shard;

namespace shard
{
	extern "C"
	{
		SHARDLIB_EXPORT ObjectInstance* shard_random_Integer(const CallState& context) noexcept(false)
		{
			std::random_device rd;
			std::mt19937 gen(rd());
			std::uniform_int_distribution<std::int64_t> distrib(LONG_MIN, LONG_MAX);

			std::int64_t value = distrib(gen);
			return context.Domain.GetGarbageCollector().FromValue(value);
		}

		SHARDLIB_EXPORT ObjectInstance* shard_random_Integer_top(const CallState& context) noexcept(false)
		{
			std::int64_t top = context.Args[0]->AsInteger();

			if (top == 0)
				return context.Collector.FromValue(0.0);

			if (top < 0)
				throw std::runtime_error("top value must be greater than zero");

			std::random_device rd;
			std::mt19937 gen(rd());
			std::uniform_int_distribution<> distrib(0, INT_MAX);

			std::int64_t value = distrib(gen);
			return context.Collector.FromValue(value);
		}

		SHARDLIB_EXPORT ObjectInstance* shard_random_Integer_bottom_top(const CallState& context) noexcept(false)
		{
			std::int64_t bottom = context.Args[0]->AsInteger();
			std::int64_t top = context.Args[1]->AsInteger();

			if (bottom == top)
				return context.Collector.FromValue(bottom);

			if (top < bottom)
				throw std::runtime_error("top edge value must be greater than bottom edge value");

			std::random_device rd;
			std::mt19937 gen(rd());
			std::uniform_int_distribution<> distrib(static_cast<int>(bottom), static_cast<int>(top));

			std::int64_t value = distrib(gen);
			return context.Collector.FromValue(value);
		}

		SHARDLIB_EXPORT ObjectInstance* shard_random_Double(const CallState& context) noexcept(false)
		{
			std::random_device rd;
			std::mt19937 gen(rd());
			std::uniform_real_distribution<> real_distrib(0.0, 1.0);

			double value = real_distrib(gen);
			return context.Collector.FromValue(value);
		}

		SHARDLIB_EXPORT ObjectInstance* shard_random_Double_top(const CallState& context) noexcept(false)
		{
			double top = context.Args[0]->AsDouble();

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

		SHARDLIB_EXPORT ObjectInstance* shard_random_Double_bottom_top(const CallState& context) noexcept(false)
		{
			double bottom = context.Args[0]->AsDouble();
			double top = context.Args[1]->AsDouble();

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

		SHARDLIB_EXPORT ObjectInstance* shard_random_Propably(const CallState& context) noexcept(false)
		{
			double chance = context.Args[0]->AsDouble();

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
	}
}
