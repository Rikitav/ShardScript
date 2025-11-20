#include <shard/parsing/analysis/DiagnosticsContext.h>
#include <shard/parsing/analysis/Diagnostic.h>
#include <shard/parsing/analysis/DiagnosticSeverity.h>
#include <shard/syntax/SyntaxToken.h>

#include <string>
#include <ostream>

using namespace shard::syntax;
using namespace shard::parsing::analysis;

void DiagnosticsContext::ReportError(SyntaxToken token, std::wstring message)
{
	AnyError = true;
	Diagnostics.push_back(Diagnostic(token, DiagnosticSeverity::Error, message));
}

void DiagnosticsContext::ReportWarning(SyntaxToken token, std::wstring message)
{
	Diagnostics.push_back(Diagnostic(token, DiagnosticSeverity::Warning, message));
}

void DiagnosticsContext::ReportInfo(SyntaxToken token, std::wstring message)
{
	Diagnostic dgnstc = Diagnostic(token, DiagnosticSeverity::Info, message);
	Diagnostics.push_back(dgnstc);
}

void DiagnosticsContext::Reset()
{
	AnyError = false;
	Diagnostics.clear();
}

void DiagnosticsContext::WriteDiagnostics(std::wostream& out)
{
	for (const Diagnostic& diag : Diagnostics)
	{
		out << severity_to_wstring(diag.Severity) << " | ";
		out << "File: '" << diag.Location.FileName << "' | ";
		out << "Line: " << diag.Location.Line << " | ";
		out << "Offset: " << diag.Location.Offset << " | ";
		out << "Word: '" << diag.Token.Word << "' | ";
		out << "Index: '" << diag.Token.Index << "' | ";
		out << diag.Description;
		out << std::endl;
	}
}
