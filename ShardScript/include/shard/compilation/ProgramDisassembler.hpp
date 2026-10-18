#pragma once
#include <shard/Definitions.hpp>

#include <ostream>

namespace shard
{
	class VirtualProgramImage;

	class SHARD_API ProgramDisassembler
	{
	public:
		void disassemble(std::wostream& out, const VirtualProgramImage& program);
	};
}
