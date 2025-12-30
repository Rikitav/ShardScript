#pragma once
#include <shard/parsing/analysis/DiagnosticsContext.h>
#include <shard/parsing/semantic/SemanticModel.h>
#include <shard/parsing/lexical/SyntaxTree.h>

namespace shard
{
	class InteractiveConsole
	{
	public:
		static void Run(shard::SyntaxTree& syntaxTree, shard::SemanticModel& semanticModel, shard::DiagnosticsContext& diagnostics);
	};
}
