#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/compilation/ProgramVirtualImage.h>

#include <ostream>

namespace shard
{
	class SHARD_API ProgramDisassembler
	{
	public:
		void Disassemble(std::wostream& out, ProgramVirtualImage& program);
	};
}
