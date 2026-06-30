#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <string>

namespace shard
{
	enum class DiagnosticSeverity
	{
		Info,
		Warning,
		Error
	};
}

SHARD_API std::wstring severity_to_wstring(const shard::DiagnosticSeverity& severity);
