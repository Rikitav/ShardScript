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
	EscapeAnalyzer escapeAnalyzer(model, pragmatic, m_diagnostics);
	escapeAnalyzer.analyze(tree);
}
