#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/semantic/SemanticModel.hpp>
#include <shard/analysis/DiagnosticsContext.hpp>
#include <shard/parsing/SyntaxTree.hpp>

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
