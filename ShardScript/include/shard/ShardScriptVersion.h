#pragma once
#include <shard/ShardScriptAPI.h>

namespace shard
{
	class SHARD_API ShardScriptVersion
	{
	public:
		static const wchar_t Version[10];

		bool IsCompatibleWith(const wchar_t* version);
	};
}