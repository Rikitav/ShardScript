#pragma once
#include <shard/parsing/semantic/SemanticModel.h>
#include <shard/parsing/analysis/DiagnosticsContext.h>
#include <shard/parsing/lexical/SyntaxTree.h>

namespace shard::parsing
{
	class SemanticAnalyzer
	{
		shard::parsing::analysis::DiagnosticsContext& Diagnostics;

	public:
		inline SemanticAnalyzer(shard::parsing::analysis::DiagnosticsContext& diagnostics)
			: Diagnostics(diagnostics) { }

		shard::parsing::semantic::SemanticModel Analyze(shard::parsing::lexical::SyntaxTree& syntaxTree);
	};
}
