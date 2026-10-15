#include <shard/semantic/SemanticAnalyzer.hpp>

#include <shard/parsing/SyntaxTree.hpp>

#include <shard/semantic/SemanticModel.hpp>
#include <shard/semantic/DeclarationCollector.hpp>
#include <shard/semantic/TypeBinder.hpp>
#include <shard/semantic/ExpressionBinder.hpp>
#include <shard/semantic/ControlFlowAnalyzer.hpp>

using namespace shard;

SemanticAnalyzer::SemanticAnalyzer(DiagnosticsContext& diagnostics)
	: m_diagnostics(diagnostics)
{ }

void SemanticAnalyzer::analyze(SyntaxTree& tree, SemanticModel& model)
{
	DeclarationCollector collector(model, m_diagnostics);
	for (const gmt::Ref<TranslationUnitSyntax>& unit : tree.get_units())
		collector.visit_translation_unit(unit.as_ptr());

	/*
	if (m_diagnostics.has_errors())
		return;
	*/

	TypeBinder typeBinder(model, m_diagnostics);
	for (const gmt::Ref<TranslationUnitSyntax>& unit : tree.get_units())
		typeBinder.visit_translation_unit(unit.as_ptr());

	/*
	if (m_diagnostics.has_errors())
		return;
	*/

	ExpressionBinder expressionBinder(model, m_diagnostics);
	for (const gmt::Ref<TranslationUnitSyntax>& unit : tree.get_units())
		expressionBinder.visit_translation_unit(unit.as_ptr());

	/*
	if (m_diagnostics.has_errors())
		return;
	*/

	ControlFlowAnalyzer flowAnalyzer(model, m_diagnostics);
	flowAnalyzer.analyze(tree);

	if (m_diagnostics.has_errors())
		return;

	// TODO: SemanticValidator pass
	// its responsibilities are being reconsidered
}
