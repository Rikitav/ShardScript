#include <shard/parsing/analysis/Diagnostic.h>
#include <shard/parsing/analysis/DiagnosticSeverity.h>
#include <shard/syntax/SyntaxToken.h>
#include <string>

using namespace shard;

Diagnostic::Diagnostic(SyntaxToken token, DiagnosticSeverity severity, std::wstring description)
	: Token(token), Severity(severity), Description(description), Location(token.Location) { }

Diagnostic::Diagnostic(const Diagnostic& other)
	: Severity(other.Severity), Description(other.Description), Token(other.Token), Location(other.Location) { }