#pragma once

namespace shard
{
	class SemanticModel;
}

namespace shard::interpreter
{
	// Registers the host-provided `stdio` namespace (print/println overloads)
	// directly into the semantic model, instead of loading a dynamic library.
	// TODO: restore dynamic library loading (system/*.dll) once the core
	// library exposes a loader API again.
	void register_standard_library(shard::SemanticModel& model);
}
