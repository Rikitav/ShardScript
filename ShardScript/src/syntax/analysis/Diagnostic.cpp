#include <shard/syntax/analysis/Diagnostic.h>
#include <shard/syntax/SyntaxToken.h>
#include <shard/syntax/analysis/TextLocation.h>
#include <string>
#include <shard/syntax/analysis/DiagnosticSeverity.h>

using namespace shard::syntax::analysis;

Diagnostic::Diagnostic(SyntaxToken token, DiagnosticSeverity severity, string description) : Token(token), Severity(severity), Description(description), Location(token.Location)
{

}