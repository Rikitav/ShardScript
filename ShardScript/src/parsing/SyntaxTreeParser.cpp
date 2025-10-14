#include <shard/parsing/SyntaxTreeParser.h>

using namespace shard::parsing;

void SyntaxTreeParser::EnsureSyntaxTree(shared_ptr<SyntaxTree> tree)
{
	if (tree->EntryPoint == nullptr)
		Diagnostics.ReportError(SyntaxToken(), "Missing script entry point");

	for (const shared_ptr<CompilationUnitSyntax> unit : tree->CompilationUnits)
		EnsureCompilationUnit(unit);
}

void SyntaxTreeParser::EnsureCompilationUnit(shared_ptr<CompilationUnitSyntax> syntax)
{
	for (const shared_ptr<MemberDeclarationSyntax> member : syntax->Members)
		EnsureMemberDeclaration(member);
}

void SyntaxTreeParser::EnsureMemberDeclaration(shared_ptr<MemberDeclarationSyntax> syntax)
{

}
