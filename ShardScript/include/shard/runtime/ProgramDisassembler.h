#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/compilation/ProgramVirtualImage.h>

#include <ostream>

namespace shard
{
	class SHARD_API ProgramDisassembler
	{
		void DisassembleAndWriteTo(std::wostream& out, ProgramVirtualImage& program);
	};
}
