#include <shard/syntax/analysis/DiagnosticsContext.h>
#include <shard/syntax/analysis/Diagnostic.h>
#include <shard/syntax/analysis/DiagnosticSeverity.h>
#include <shard/syntax/SyntaxToken.h>
#include <string>

using namespace shard::syntax::analysis;

void DiagnosticsContext::ReportError(SyntaxToken token, string message)
{
	AnyError = true;
	Diagnostics.push_back(Diagnostic(token, DiagnosticSeverity::Error, message));
}

void DiagnosticsContext::ReportWarning(SyntaxToken token, string message)
{
	Diagnostics.push_back(Diagnostic(token, DiagnosticSeverity::Warning, message));
}

void DiagnosticsContext::ReportInfo(SyntaxToken token, string message)
{
	Diagnostic dgnstc = Diagnostic(token, DiagnosticSeverity::Info, message);
	Diagnostics.push_back(dgnstc);
}

void DiagnosticsContext::Reset()
{
	AnyError = false;
	Diagnostics.clear();
}
