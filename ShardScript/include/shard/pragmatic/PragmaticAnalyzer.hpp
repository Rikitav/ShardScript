#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/Diagnostics.hpp>
#include <shard/parsing/SyntaxTree.hpp>

#include <shard/semantic/SemanticModel.hpp>

#include <shard/pragmatic/PragmaticModel.hpp>

namespace shard
{
	class SHARD_API PragmaticAnalyzer
	{
		DiagnosticsContext& m_diagnostics;

	public:
		PragmaticAnalyzer(DiagnosticsContext& diagnostics);

		void analyze(SyntaxTree& tree, SemanticModel& model, PragmaticModel& pragmatic);
	};
}
