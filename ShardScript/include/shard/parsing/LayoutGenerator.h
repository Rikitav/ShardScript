#pragma once
#include <shard/parsing/semantic/SemanticModel.h>
#include <shard/parsing/analysis/DiagnosticsContext.h>

namespace shard::parsing
{
	class LayoutGenerator
	{
	private:
		shard::parsing::analysis::DiagnosticsContext& Diagnostics;

	public:
		LayoutGenerator(shard::parsing::analysis::DiagnosticsContext& diagnostics)
			: Diagnostics(diagnostics) { }

		void Generate(shard::parsing::semantic::SemanticModel& syntaxTree);
	};
}
