#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/semantic/SemanticModel.hpp>
#include <shard/analysis/DiagnosticsContext.hpp>
#include <shard/semantic/symbols/TypeSymbol.hpp>

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
