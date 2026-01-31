#pragma once
#include <shard/compilation/ProgramVirtualImage.h>

namespace shard
{
	class ByteCodeGenerator
	{
	public:
		inline ByteCodeGenerator() { }

		void EmitNop(std::vector<std::byte> store);
	};
}
