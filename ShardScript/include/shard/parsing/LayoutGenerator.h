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
		DiagnosticsContext& Diagnostics;

	public:
		LayoutGenerator(DiagnosticsContext& diagnostics)
			: Diagnostics(diagnostics) { }

		void Generate(SemanticModel& semanticModel);

	private:
		void FixObjectLayout(SemanticModel& semanticModel, TypeSymbol* objectInfo);
	};
}
