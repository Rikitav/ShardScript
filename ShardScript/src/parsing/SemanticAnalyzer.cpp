#include <shard/syntax/SyntaxToken.h>
#include <shard/parsing/SemanticAnalyzer.h>
#include <shard/parsing/SyntaxTree.h>

#include <shard/parsing/semantic/SemanticModel.h>
#include <shard/parsing/semantic/visiting/DeclarationCollector.h>
#include <shard/parsing/semantic/visiting/TypeBinder.h>
#include <shard/parsing/semantic/visiting/ExpressionBinder.h>

using namespace shard;

void SemanticAnalyzer::Analyze(SyntaxTree& syntaxTree, SemanticModel& semanticModel)
{
	DeclarationCollector collector(semanticModel, Diagnostics);
	collector.VisitSyntaxTree(syntaxTree);

	TypeBinder typeBinder(semanticModel, Diagnostics);
	typeBinder.VisitSyntaxTree(syntaxTree);

	ExpressionBinder expressionBinder(semanticModel, Diagnostics);
	expressionBinder.VisitSyntaxTree(syntaxTree);
}
