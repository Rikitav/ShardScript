#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/compilation/ProgramVirtualImage.h>

namespace shard
{
	class SHARD_API VirtualMachine
	{
		ProgramVirtualImage& Program;

	public:
		inline VirtualMachine(ProgramVirtualImage& program)
			: Program(program) { }

		void Run();
	};
}