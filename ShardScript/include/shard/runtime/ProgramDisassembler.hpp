#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/compilation/ProgramVirtualImage.hpp>

#include <ostream>

namespace shard
{
	class SHARD_API ProgramDisassembler
	{
	public:
		void Disassemble(std::wostream& out, ProgramVirtualImage& program);
	};
}
