#pragma once
#include <shard/ShardScriptAPI.h>

namespace shard
{
	class SHARD_API ShardScriptVersion
	{
	public:
		static const int Major;
		static const int Minor;

		bool IsCompatibleWith(const int major, const int minor);
	};
}