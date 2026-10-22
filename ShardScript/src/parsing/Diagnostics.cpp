#include <shard/parsing/Diagnostics.hpp>

#include <gmt/Containers.hpp>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <locale>
#include <string>
#include <iostream>

using namespace shard;

namespace
{
	constexpr wchar_t FG_RED[] = L"\x1b[31m";
	constexpr wchar_t FG_YELLOW[] = L"\x1b[33m";
	constexpr wchar_t FG_BLUE[] = L"\x1b[34m";
	constexpr wchar_t FG_CYAN[] = L"\x1b[36m";
	constexpr wchar_t FG_LIGHT_GRAY[] = L"\x1b[37m";
	constexpr wchar_t ST_RESET[] = L"\x1b[0m";

	const wchar_t* severity_text(const DiagnosticSeverity severity)
	{
		switch (severity)
		{
			case DiagnosticSeverity::Warning:	return L"warning";
			case DiagnosticSeverity::Info:		return L"info";
			default:							return L"error";
		}
	}

	const wchar_t* severity_color(const DiagnosticSeverity severity)
	{
		switch (severity)
		{
			case DiagnosticSeverity::Warning:	return FG_YELLOW;
			case DiagnosticSeverity::Info:		return FG_BLUE;
			default:							return FG_RED;
		}
	}

	std::wifstream* open_source_file(SourceFileCache& cache, const std::wstring_view filePath)
	{
		auto cached = cache.find(filePath);
		if (cached != cache.end())
			return &cached->second;

		auto [it, inserted] = cache.emplace(filePath, std::wifstream {});
		std::wifstream& in = it->second;

		in.open(std::filesystem::path(filePath));
		in.imbue(std::locale::classic());

		return &in;
	}

	gmt::wstring read_source_line(SourceFileCache& cache, const std::wstring_view filePath, const std::int32_t targetLine)
	{
		std::wifstream& in = *open_source_file(cache, filePath);
		if (!in.is_open())
			return L"";

		in.clear();
		in.seekg(0);

		gmt::wstring line;
		std::int32_t currentLine = 1;
		while (currentLine < targetLine && std::getline(in, line))
			++currentLine;

		if (!std::getline(in, line))
			return L"";

		if (!line.empty() && line.back() == L'\r')
			line.pop_back();

		return line;
	}
}

Diagnostic::Diagnostic(
	const SyntaxToken& token,
	DiagnosticSeverity severity,
	const gmt::wstring& description
) :
	m_severity(severity),
	m_description(description),
	m_blameToken(token),
	m_location(token.get_location())
{ }

Diagnostic::Diagnostic(
	TextLocation location,
	DiagnosticSeverity severity,
	const gmt::wstring& description
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
	const gmt::wstring& description
) :
	m_severity(severity),
	m_description(description),
	m_blameToken(token),
	m_location(location)
{ }

void DiagnosticsContext::report_error(const SyntaxToken& token, const gmt::wstring& description)
{
	m_anyError = true;
	m_diagnostics.push_back(Diagnostic(token, DiagnosticSeverity::Error, description));
}

void DiagnosticsContext::report_error(const SyntaxToken& token, string_t description)
{
	m_anyError = true;
	m_diagnostics.push_back(Diagnostic(token, DiagnosticSeverity::Error, description));
}

void DiagnosticsContext::report_error(const TextLocation& location, const gmt::wstring& description)
{
	m_anyError = true;
	m_diagnostics.push_back(Diagnostic(location, DiagnosticSeverity::Error, description));
}

void DiagnosticsContext::report_error(const TextLocation& location, string_t description)
{
	m_anyError = true;
	m_diagnostics.push_back(Diagnostic(location, DiagnosticSeverity::Error, description));
}

void DiagnosticsContext::report_warning(const SyntaxToken& token, const gmt::wstring& description)
{
	m_diagnostics.push_back(Diagnostic(token, DiagnosticSeverity::Warning, description));
}

void DiagnosticsContext::report_warning(const SyntaxToken& token, string_t description)
{
	m_diagnostics.push_back(Diagnostic(token, DiagnosticSeverity::Warning, description));
}

void DiagnosticsContext::report_warning(const TextLocation& location, const gmt::wstring& description)
{
	m_anyWarns = true;
	m_diagnostics.push_back(Diagnostic(location, DiagnosticSeverity::Warning, description));
}

void DiagnosticsContext::report_warning(const TextLocation& location, string_t description)
{
	m_anyWarns = true;
	m_diagnostics.push_back(Diagnostic(location, DiagnosticSeverity::Warning, description));
}

void DiagnosticsContext::report_info(const SyntaxToken& token, const gmt::wstring& description)
{
	Diagnostic dgnstc = Diagnostic(token, DiagnosticSeverity::Info, description);
	m_diagnostics.push_back(dgnstc);
}

void DiagnosticsContext::report_info(const SyntaxToken& token, string_t description)
{
	Diagnostic dgnstc = Diagnostic(token, DiagnosticSeverity::Info, description);
	m_diagnostics.push_back(dgnstc);
}

void DiagnosticsContext::report_info(const TextLocation& location, const gmt::wstring& description)
{
	m_diagnostics.push_back(Diagnostic(location, DiagnosticSeverity::Info, description));
}

void DiagnosticsContext::report_info(const TextLocation& location, string_t description)
{
	m_diagnostics.push_back(Diagnostic(location, DiagnosticSeverity::Info, description));
}

std::span<const Diagnostic> DiagnosticsContext::get_diagnostics() const
{
	return std::span(m_diagnostics.data(), m_diagnostics.data() + m_diagnostics.size());
}

bool DiagnosticsContext::has_errors() const
{
	return m_anyError;
}

bool DiagnosticsContext::has_warns() const
{
	return m_anyWarns;
}

void DiagnosticsContext::reset()
{
	m_diagnostics.clear();
	m_anyError = false;
	m_anyWarns = false;
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

void DiagnosticsContext::write_diagnostics_pretty(std::wostream& out) const
{
	SourceFileCache sourceFiles;
	for (const Diagnostic& diagnostic : m_diagnostics)
		write_diagnostic_pretty(out, diagnostic, sourceFiles);
}

void DiagnosticsContext::write_diagnostic_pretty(std::wostream& out, const Diagnostic& diagnostic) const
{
	SourceFileCache sourceFiles;
	write_diagnostic_pretty(out, diagnostic, sourceFiles);
}

void DiagnosticsContext::write_diagnostic_pretty(std::wostream& out, const Diagnostic& diagnostic, SourceFileCache& sourceFiles) const
{
	const TextLocation& location = diagnostic.get_location();
	const DiagnosticSeverity severity = diagnostic.get_severity();
	const wchar_t* color = severity_color(severity);

	out << color << severity_text(severity) << ST_RESET
		<< L": " << diagnostic.get_description() << std::endl;

	out << FG_LIGHT_GRAY << L" --> " << ST_RESET
		<< FG_CYAN << location.get_filename() << ST_RESET
		<< FG_LIGHT_GRAY << L":" << location.get_line() << L":" << location.get_offset() << ST_RESET << std::endl;

	out << FG_LIGHT_GRAY << L"    |" << ST_RESET << std::endl;

	gmt::wstring sourceLine = read_source_line(sourceFiles, location.get_filename(), location.get_line());
	if (sourceLine.empty())
		return;

	constexpr int MaxVisibleLineLength = 160;
	constexpr int CaretWindowRadius = 60;

	int start = (std::max)(0, location.get_offset() - 1);
	int length = (std::max)(1, location.get_length());

	if (start > static_cast<int>(sourceLine.length()))
		start = static_cast<int>(sourceLine.length());

	if (start + length > static_cast<int>(sourceLine.length()))
		length = static_cast<int>(sourceLine.length()) - start;

	// if the source line is extremely long, show only a window around the diagnostic so the output stays readable
	gmt::wstring displayedLine = sourceLine;
	int lineOffset = 0;
	bool trimmedPrefix = false;
	bool trimmedSuffix = false;

	if (static_cast<int>(sourceLine.length()) > MaxVisibleLineLength)
	{
		int windowStart = (std::max)(0, start - CaretWindowRadius);
		int windowEnd = (std::min)(static_cast<int>(sourceLine.length()), start + length + CaretWindowRadius);

		if (windowStart > 0)
		{
			trimmedPrefix = true;
			windowStart = (std::max)(0, windowStart - 4);
		}

		if (windowEnd < static_cast<int>(sourceLine.length()))
		{
			trimmedSuffix = true;
			windowEnd = (std::min)(static_cast<int>(sourceLine.length()), windowEnd + 4);
		}

		displayedLine = sourceLine.substr(windowStart, windowEnd - windowStart);
		lineOffset = windowStart;
	}

	out << FG_LIGHT_GRAY << std::setw(3) << location.get_line() << L" | " << ST_RESET;
	if (trimmedPrefix)
		out << L"...";

	out << displayedLine;
	if (trimmedSuffix)
		out << L"...";

	out << std::endl;

	int caretStart = start - lineOffset;
	if (trimmedPrefix)
		caretStart += 3;

	out << FG_LIGHT_GRAY << L"    | " << ST_RESET
		<< gmt::wstring(caretStart, L' ')
		<< color << gmt::wstring(length, L'^') << ST_RESET << std::endl;
}

string_t shard::severity_to_wstring(const DiagnosticSeverity severity)
{
	switch (severity)
	{
		case DiagnosticSeverity::Info:		return L"Info";
		case DiagnosticSeverity::Warning:	return L"Warn";
		case DiagnosticSeverity::Error:		return L"Error";
		default:							return L"";
	}
}
