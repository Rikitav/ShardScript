#include <shard/parsing/Diagnostics.hpp>

#include <string>
#include <iostream>

using namespace shard;

Diagnostic::Diagnostic(
	const SyntaxToken& token,
	DiagnosticSeverity severity,
	const std::wstring& description
) :
	m_severity(severity),
	m_description(description),
	m_blameToken(token),
	m_location(token.get_location())
{ }

Diagnostic::Diagnostic(
	TextLocation location,
	DiagnosticSeverity severity,
	const std::wstring& description
) :
	m_severity(severity),
	m_description(description),
	m_blameToken(),
	m_location(location)
{ }

Diagnostic::Diagnostic(
	TextLocation location,
	const SyntaxToken& token,
	DiagnosticSeverity severity,
	const std::wstring& description
) :
	m_severity(severity),
	m_description(description),
	m_blameToken(token),
	m_location(location)
{ }

void DiagnosticsContext::report_error(const SyntaxToken& token, const std::wstring& description)
{
	m_anyError = true;
	m_diagnostics.push_back(Diagnostic(token, DiagnosticSeverity::Error, description));
}

void DiagnosticsContext::report_error(const SyntaxToken& token, string_t description)
{
	m_anyError = true;
	m_diagnostics.push_back(Diagnostic(token, DiagnosticSeverity::Error, description));
}

void DiagnosticsContext::report_warning(const SyntaxToken& token, const std::wstring& description)
{
	m_diagnostics.push_back(Diagnostic(token, DiagnosticSeverity::Warning, description));
}

void DiagnosticsContext::report_warning(const SyntaxToken& token, string_t description)
{
	m_diagnostics.push_back(Diagnostic(token, DiagnosticSeverity::Warning, description));
}

void DiagnosticsContext::report_info(const SyntaxToken& token, const std::wstring& description)
{
	Diagnostic dgnstc = Diagnostic(token, DiagnosticSeverity::Info, description);
	m_diagnostics.push_back(dgnstc);
}

void DiagnosticsContext::report_info(const SyntaxToken& token, string_t description)
{
	Diagnostic dgnstc = Diagnostic(token, DiagnosticSeverity::Info, description);
	m_diagnostics.push_back(dgnstc);
}

std::span<const Diagnostic> DiagnosticsContext::get_diagnostics() const
{
	return std::span<const Diagnostic>(m_diagnostics.data(), m_diagnostics.data() + m_diagnostics.size());
}

void DiagnosticsContext::reset()
{
	m_diagnostics.clear();
	m_anyError = false;
}

void DiagnosticsContext::write_diagnostics(std::wostream& out) const
{
	for (const Diagnostic& diag : m_diagnostics)
	{
		TextLocation location = diag.get_location();
		SyntaxToken blameToken = diag.get_blame_token();

		out << severity_to_wstring(diag.get_severity()) << L" | ";
		out << L"Location: '" << location.get_filename() << L":" << location.get_line()	<< L":"	<< location.get_offset() << L" | ";

		if (!blameToken.is_missing())
			out << "Word: '" << blameToken.get_lexeme() << "' | ";

		out << diag.get_description();
	}

	out << std::flush;
}

string_t severity_to_wstring(const DiagnosticSeverity severity)
{
	switch (severity)
	{
		case DiagnosticSeverity::Info:		return L"Info";
		case DiagnosticSeverity::Warning:	return L"Warn";
		case DiagnosticSeverity::Error:		return L"Error";
		default:							return L"";
	}
}
