#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/parsing/semantic/SemanticModel.h>
#include <shard/parsing/analysis/DiagnosticsContext.h>
#include <shard/syntax/symbols/TypeSymbol.h>

namespace shard::parsing
{
	class SHARD_API LayoutGenerator
	{
	private:
		shard::parsing::analysis::DiagnosticsContext& Diagnostics;

	public:
		LayoutGenerator(shard::parsing::analysis::DiagnosticsContext& diagnostics)
			: Diagnostics(diagnostics) { }

		void Generate(shard::parsing::semantic::SemanticModel& semanticModel);

	private:
		void FixObjectLayout(shard::parsing::semantic::SemanticModel& semanticModel, shard::syntax::symbols::TypeSymbol* objectInfo);
	};
}
