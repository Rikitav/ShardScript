#include <shard/parsing/analysis/Diagnostic.h>
#include <shard/parsing/analysis/DiagnosticSeverity.h>
#include <shard/syntax/SyntaxToken.h>
#include <string>

using namespace std;
using namespace shard::syntax;
using namespace shard::parsing::analysis;

Diagnostic::Diagnostic(SyntaxToken token, DiagnosticSeverity severity, string description)
	: Token(token), Severity(severity), Description(description), Location(token.Location) { }