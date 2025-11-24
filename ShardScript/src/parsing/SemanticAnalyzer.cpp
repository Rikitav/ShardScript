#include <shard/syntax/SyntaxToken.h>
#include <shard/parsing/SemanticAnalyzer.h>
#include <shard/parsing/lexical/SyntaxTree.h>

#include <shard/parsing/semantic/SemanticModel.h>
#include <shard/parsing/visiting/DeclarationCollector.h>
#include <shard/parsing/visiting/TypeBinder.h>
#include <shard/parsing/visiting/ExpressionBinder.h>

using namespace shard::syntax;
using namespace shard::parsing;
using namespace shard::parsing::lexical;
using namespace shard::parsing::semantic;
using namespace shard::parsing::analysis;

void SemanticAnalyzer::Analyze(SyntaxTree& syntaxTree, SemanticModel& semanticModel)
{
	DeclarationCollector collector(semanticModel, Diagnostics);
	collector.VisitSyntaxTree(syntaxTree);

	TypeBinder typeBinder(semanticModel, Diagnostics);
	typeBinder.VisitSyntaxTree(syntaxTree);

	ExpressionBinder expressionBinder(semanticModel, Diagnostics);
	expressionBinder.VisitSyntaxTree(syntaxTree);
}
