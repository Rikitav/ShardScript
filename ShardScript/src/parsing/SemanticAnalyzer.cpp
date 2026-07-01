#include <shard/parsing/SyntaxToken.hpp>
#include <shard/semantic/SemanticAnalyzer.hpp>
#include <shard/parsing/SyntaxTree.hpp>

#include <shard/semantic/SemanticModel.hpp>
#include <shard/semantic/DeclarationCollector.hpp>
#include <shard/semantic/TypeBinder.hpp>
#include <shard/semantic/ExpressionBinder.hpp>
#include <shard/semantic/SemanticValidator.hpp>

using namespace shard;

SemanticAnalyzer::SemanticAnalyzer(DiagnosticsContext& diagnostics) : Diagnostics(diagnostics)
{
	TopScope = new SemanticScope(nullptr, SymbolTable::Global::Scope);
}

SemanticAnalyzer::~SemanticAnalyzer()
{
	delete TopScope;
}

void SemanticAnalyzer::AddSymbol(SyntaxSymbol* symbol)
{
	TopScope->DeclareSymbol(symbol);
}

void SemanticAnalyzer::Analyze(SyntaxTree& syntaxTree, SemanticModel& semanticModel)
{
	DeclarationCollector collector(semanticModel, Diagnostics);
	collector.PushScopeStack(TopScope);
	collector.VisitSyntaxTree(syntaxTree);

	TypeBinder typeBinder(semanticModel, Diagnostics);
	typeBinder.PushScopeStack(TopScope);
	typeBinder.VisitSyntaxTree(syntaxTree);

	ExpressionBinder expressionBinder(semanticModel, Diagnostics);
	expressionBinder.PushScopeStack(TopScope);
	expressionBinder.VisitSyntaxTree(syntaxTree);

	// Final cross-symbol validation pass. This also catches interface
	// implementation mistakes in symbols created by native libraries.
	SemanticValidator::ValidateAllInterfaceImplementations(semanticModel, Diagnostics);
}
