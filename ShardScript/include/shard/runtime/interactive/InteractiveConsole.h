#pragma once
#include <shard/parsing/analysis/DiagnosticsContext.h>
#include <shard/parsing/semantic/SemanticModel.h>
#include <shard/parsing/lexical/SyntaxTree.h>

namespace shard::runtime
{
	class InteractiveConsole
	{
	public:
		static void Run(shard::parsing::lexical::SyntaxTree& syntaxTree, shard::parsing::semantic::SemanticModel& semanticModel, shard::parsing::analysis::DiagnosticsContext& diagnostics);
	};
}
