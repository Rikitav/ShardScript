#pragma once
#include <shard/parsing/LexicalAnalyzer.h>
#include <shard/parsing/SemanticAnalyzer.h>
#include <shard/parsing/semantic/SemanticModel.h>
#include <shard/parsing/analysis/DiagnosticsContext.h>

using namespace shard::parsing::semantic;

namespace shard::framework
{
	class FrameworkLoader
	{
	public:
		static void Load(shard::parsing::LexicalAnalyzer& lexer, shard::parsing::semantic::SemanticModel& semanticModel, shard::parsing::analysis::DiagnosticsContext& diagnostics);

	private:
		static void ResolvePrmitives(shard::parsing::semantic::SemanticModel& semanticModel);
		static void ResolveGlobalMethods(shard::parsing::semantic::SemanticModel& semanticModel);
	};
}
