#include <shard/parsing/visiting/TypeBinder.h>
#include <shard/syntax/nodes/CompilationUnitSyntax.h>
#include <shard/syntax/nodes/MemberDeclarationSyntax.h>
#include <shard/syntax/nodes/Directives/UsingDirectiveSyntax.h>

using namespace shard::parsing;
using namespace shard::syntax::nodes;

void TypeBinder::VisitCompilationUnit(CompilationUnitSyntax* node)
{
	for (UsingDirectiveSyntax* directive : node->Usings)
		VisitUsingDirective(directive);

	for (MemberDeclarationSyntax* member : node->Members)
		VisitTypeDeclaration(member);
}