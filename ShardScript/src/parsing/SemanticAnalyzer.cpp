#include <shard/syntax/SyntaxToken.hpp>
#include <shard/parsing/SemanticAnalyzer.hpp>
#include <shard/parsing/SyntaxTree.hpp>

#include <shard/parsing/semantic/SemanticModel.hpp>
#include <shard/parsing/semantic/visiting/DeclarationCollector.hpp>
#include <shard/parsing/semantic/visiting/TypeBinder.hpp>
#include <shard/parsing/semantic/visiting/ExpressionBinder.hpp>

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
}
