#pragma once
#include <shard/compilation/ProgramVirtualImage.h>

namespace shard
{
	class VirtualMachine
	{
		ProgramVirtualImage& Program;

	public:
		inline VirtualMachine(ProgramVirtualImage& program)
			: Program(program) { }

		void Run();
	};
}