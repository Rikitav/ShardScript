#include <shard/parsing/analysis/DiagnosticsContext.hpp>
#include <shard/parsing/analysis/Diagnostic.hpp>
#include <shard/parsing/analysis/DiagnosticSeverity.hpp>
#include <shard/syntax/SyntaxToken.hpp>

#include <string>
#include <ostream>
#include <stdexcept>
#include <memory>
#include <string_view>

using namespace shard;

diagnostics_exception::diagnostics_exception(const char* message) : std::runtime_error(message) { }
diagnostics_exception::diagnostics_exception(std::string& message) : std::runtime_error(message) { }

void DiagnosticsContext::ReportError(SyntaxToken token, std::wstring message)
{
	AnyError = true;
	Diagnostics.push_back(Diagnostic(token, DiagnosticSeverity::Error, message));
}

void DiagnosticsContext::ReportError(SyntaxToken token, const wchar_t* message)
{
	AnyError = true;
	Diagnostics.push_back(Diagnostic(token, DiagnosticSeverity::Error, message));
}

void DiagnosticsContext::ReportWarning(SyntaxToken token, std::wstring message)
{
	Diagnostics.push_back(Diagnostic(token, DiagnosticSeverity::Warning, message));
}

void DiagnosticsContext::ReportWarning(SyntaxToken token, const wchar_t* message)
{
	Diagnostics.push_back(Diagnostic(token, DiagnosticSeverity::Warning, message));
}

void DiagnosticsContext::ReportInfo(SyntaxToken token, std::wstring message)
{
	Diagnostic dgnstc = Diagnostic(token, DiagnosticSeverity::Info, message);
	Diagnostics.push_back(dgnstc);
}

void DiagnosticsContext::ReportInfo(SyntaxToken token, const wchar_t* message)
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
