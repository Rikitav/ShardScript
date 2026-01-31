#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/parsing/semantic/SemanticModel.h>
#include <shard/parsing/analysis/DiagnosticsContext.h>
#include <shard/parsing/SyntaxTree.h>

namespace shard
{
	class SHARD_API SemanticAnalyzer
	{
		shard::DiagnosticsContext& Diagnostics;

	public:
		inline SemanticAnalyzer(shard::DiagnosticsContext& diagnostics)
			: Diagnostics(diagnostics) { }

		void Analyze(shard::SyntaxTree& syntaxTree, shard::SemanticModel& semanticModel);
	};
}
