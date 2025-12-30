#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/parsing/semantic/SemanticModel.h>
#include <shard/parsing/analysis/DiagnosticsContext.h>
#include <shard/syntax/symbols/TypeSymbol.h>

namespace shard
{
	class SHARD_API LayoutGenerator
	{
	private:
		shard::DiagnosticsContext& Diagnostics;

	public:
		LayoutGenerator(shard::DiagnosticsContext& diagnostics)
			: Diagnostics(diagnostics) { }

		void Generate(shard::SemanticModel& semanticModel);

	private:
		void FixObjectLayout(shard::SemanticModel& semanticModel, shard::TypeSymbol* objectInfo);
	};
}
