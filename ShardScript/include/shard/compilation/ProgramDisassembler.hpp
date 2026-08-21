#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/semantic/symbols/MethodSymbol.hpp>
#include <shard/compilation/ProgramVirtualImage.hpp>
#include <shard/CompilationContext.hpp>

#include <ostream>

namespace shard
{
	class SHARD_API ProgramDisassembler
	{
	public:
		void Disassemble(std::wostream& out, const CompilationContext& compiler);
		void Disassemble(std::wostream& out, const ProgramVirtualImage& program);
		void Disassemble(std::wostream& out, const MethodSymbol* method);
	};
}
