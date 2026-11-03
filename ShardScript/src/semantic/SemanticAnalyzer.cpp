#include <shard/semantic/SemanticAnalyzer.hpp>

#include <shard/parsing/SyntaxTree.hpp>

#include <shard/semantic/SemanticModel.hpp>
#include <shard/semantic/DeclarationCollector.hpp>
#include <shard/semantic/TypeBinder.hpp>
#include <shard/semantic/ExpressionBinder.hpp>
#include <shard/semantic/NullableFlowAnalyzer.hpp>
#include <shard/semantic/ControlFlowAnalyzer.hpp>

using namespace shard;

SemanticAnalyzer::SemanticAnalyzer(DiagnosticsContext& diagnostics)
	: m_diagnostics(diagnostics)
{ }

void SemanticAnalyzer::analyze(SyntaxTree& tree, SemanticModel& model)
{
	DeclarationCollector collector(model, m_diagnostics);
	collector.visit_syntax_tree(tree);

	TypeBinder typeBinder(model, m_diagnostics);
	typeBinder.visit_syntax_tree(tree);

	ExpressionBinder expressionBinder(model, m_diagnostics);
	expressionBinder.visit_syntax_tree(tree);

	ControlFlowAnalyzer flowAnalyzer(model, m_diagnostics);
	flowAnalyzer.analyze(tree);

	if (m_diagnostics.has_errors())
		return;

	NullableFlowAnalyzer nullableFlowAnalyzer(model, m_diagnostics);
	nullableFlowAnalyzer.analyze(tree);
}