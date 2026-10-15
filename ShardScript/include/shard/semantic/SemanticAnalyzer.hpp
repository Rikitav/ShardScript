#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/Diagnostics.hpp>

namespace shard
{
	class SemanticModel;
	class SyntaxTree;

	class SHARD_API SemanticAnalyzer
	{
		DiagnosticsContext& m_diagnostics;

	public:
		SemanticAnalyzer(DiagnosticsContext& diagnostics);

		void analyze(SyntaxTree& tree, SemanticModel& model);
	};
}
