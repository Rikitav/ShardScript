#include <shard/pragmatic/PragmaticAnalyzer.hpp>

#include <shard/parsing/SyntaxTree.hpp>

#include <shard/semantic/SemanticModel.hpp>
#include <shard/pragmatic/EscapeAnalyzer.hpp>
#include <shard/pragmatic/PragmaticModel.hpp>

using namespace shard;

PragmaticAnalyzer::PragmaticAnalyzer(DiagnosticsContext& diagnostics)
	: m_diagnostics(diagnostics)
{ }

void PragmaticAnalyzer::analyze(SyntaxTree& tree, SemanticModel& model, PragmaticModel& pragmatic)
{
	// TODO: port AsyncHoistingPass + AsyncAnalysisPass here (before escape
	// analysis, so it sees awaits normalized to statement level); emission
	// stays with the future translation layer

	EscapeAnalyzer escapeAnalyzer(model, pragmatic, m_diagnostics);
	escapeAnalyzer.analyze(tree);
}
