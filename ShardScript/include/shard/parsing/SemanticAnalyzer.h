#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/parsing/semantic/SemanticModel.h>
#include <shard/parsing/analysis/DiagnosticsContext.h>
#include <shard/parsing/SyntaxTree.h>

namespace shard
{
	class SHARD_API SemanticAnalyzer
	{
		DiagnosticsContext& Diagnostics;
		SemanticScope* TopScope;

	public:
		SemanticAnalyzer(DiagnosticsContext& diagnostics);
		~SemanticAnalyzer();

		void AddSymbol(SyntaxSymbol* symbol);
		void Analyze(SyntaxTree& syntaxTree, SemanticModel& semanticModel);
	};
}
