#include <shard/syntax/SyntaxToken.h>
#include <shard/parsing/SemanticAnalyzer.h>
#include <shard/parsing/lexical/SyntaxTree.h>

#include <shard/parsing/semantic/SemanticModel.h>
#include <shard/parsing/visiting/DeclarationCollector.h>
#include <shard/parsing/visiting/TypeBinder.h>
#include <shard/parsing/visiting/ExpressionBinder.h>

using namespace std;
using namespace shard::syntax;
using namespace shard::parsing;
using namespace shard::parsing::lexical;
using namespace shard::parsing::semantic;
using namespace shard::parsing::analysis;

SemanticModel SemanticAnalyzer::Analyze(SyntaxTree& syntaxTree)
{
	SemanticModel semanticModel = SemanticModel(syntaxTree);
	semanticModel.Table->ResolvePrmitives();
	semanticModel.Table->ResolveGlobalMethods();

	DeclarationCollector collector(semanticModel.Table, Diagnostics);
	collector.VisitSyntaxTree(syntaxTree);

	TypeBinder typeBinder(semanticModel.Table, Diagnostics);
	typeBinder.VisitSyntaxTree(syntaxTree);

	ExpressionBinder expressionBinder(semanticModel.Table, Diagnostics);
	expressionBinder.VisitSyntaxTree(syntaxTree);

	if (semanticModel.Table->EntryPointCandidates.empty())
	{
		Diagnostics.ReportError(SyntaxToken(), "Entry point for script not found");
	}
	
	if (semanticModel.Table->EntryPointCandidates.size() > 1)
	{
		Diagnostics.ReportError(SyntaxToken(), "model has multiple entry points");
	}

	return semanticModel;
}
