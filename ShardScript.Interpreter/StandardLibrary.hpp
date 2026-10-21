#pragma once
#include <shard/semantic/SemanticModel.hpp>

namespace shard::interpreter
{
	// Registers the host-provided `stdio` namespace (print/println overloads)
	// directly into the semantic model. Dynamic library loading is restored
	// (see CompilationContext::add_lib and the system/ directory discovery in
	// main), but no compiled stdlib DLLs exist yet, so stdio stays embedded
	// until a ShardScript.Framework-style project ships them.
	void register_standard_library(shard::SemanticModel& model);
}
