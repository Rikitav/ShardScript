#include <shard/syntax/SyntaxToken.h>
#include <shard/parsing/SemanticAnalyzer.h>
#include <shard/parsing/SyntaxTree.h>

#include <shard/parsing/semantic/SemanticModel.h>
#include <shard/parsing/semantic/visiting/DeclarationCollector.h>
#include <shard/parsing/semantic/visiting/TypeBinder.h>
#include <shard/parsing/semantic/visiting/ExpressionBinder.h>

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
