#include <shard/parsing/SemanticAnalyzer.h>
#include <shard/parsing/lexical/SyntaxTree.h>

#include <shard/parsing/semantic/SemanticModel.h>
#include <shard/parsing/visiting/DeclarationCollector.h>
#include <shard/parsing/visiting/TypeBinder.h>

using namespace std;
using namespace shard::parsing;
using namespace shard::parsing::lexical;
using namespace shard::parsing::semantic;
using namespace shard::parsing::analysis;

SemanticModel SemanticAnalyzer::Analyze(SyntaxTree& syntaxTree)
{
	SemanticModel semanticModel = SemanticModel(syntaxTree);
	semanticModel.Table->ResolvePrmitives();

	DeclarationCollector collector(semanticModel.Table, Diagnostics);
	collector.VisitSyntaxTree(syntaxTree);

	TypeBinder binder(semanticModel.Table, Diagnostics);
	binder.VisitSyntaxTree(syntaxTree);

	return semanticModel;
}
