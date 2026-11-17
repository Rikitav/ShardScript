#include <shard/parsing/Diagnostics.hpp>

#include <shard/lexical/FileReader.hpp>

#include <gmt/Containers.hpp>

#include <algorithm>
#include <filesystem>
#include <iomanip>
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

	const gmt::vector<gmt::wstring>& load_source_lines(SourceFileCache& cache, const std::wstring_view filePath)
	{
		auto cached = cache.find(filePath);
		if (cached != cache.end())
			return cached->second;

		auto [it, inserted] = cache.emplace(filePath, gmt::vector<gmt::wstring>{});
		gmt::vector<gmt::wstring>& lines = it->second;

		// decoded exactly like the lexer decodes it, otherwise a caret would sit on
		// the wrong character of a non-ASCII line
		gmt::wstring text;

		try
		{
			FileReader reader{gmt::wstring(filePath)};

			wchar_t ch = 0;
			while (reader.read_next(ch))
				text.push_back(ch);
		}
		catch (...)
		{
			return lines;
		}

		gmt::wstring line;
		for (const wchar_t ch : text)
		{
			if (ch == L'\n')
			{
				lines.push_back(line);
				line.clear();
				continue;
			}

			if (ch != L'\r')
				line.push_back(ch);
		}

		lines.push_back(line);
		return lines;
	}

	gmt::wstring read_source_line(SourceFileCache& cache, const std::wstring_view filePath, const std::int32_t targetLine)
	{
		const gmt::vector<gmt::wstring>& lines = load_source_lines(cache, filePath);
		if (targetLine < 1 || static_cast<std::size_t>(targetLine) > lines.size())
			return L"";

		return lines[static_cast<std::size_t>(targetLine) - 1];
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

	if (start > static_cast<int>(sourceLine.size()))
		start = static_cast<int>(sourceLine.size());

	if (start + length > static_cast<int>(sourceLine.size()))
		length = static_cast<int>(sourceLine.size()) - start;

	// if the source line is extremely long, show only a window around the diagnostic so the output stays readable
	gmt::wstring displayedLine = sourceLine;
	int lineOffset = 0;
	bool trimmedPrefix = false;
	bool trimmedSuffix = false;

	if (static_cast<int>(sourceLine.size()) > MaxVisibleLineLength)
	{
		int windowStart = (std::max)(0, start - CaretWindowRadius);
		int windowEnd = (std::min)(static_cast<int>(sourceLine.size()), start + length + CaretWindowRadius);

		if (windowStart > 0)
		{
			trimmedPrefix = true;
			windowStart = (std::max)(0, windowStart - 4);
		}

		if (windowEnd < static_cast<int>(sourceLine.size()))
		{
			trimmedSuffix = true;
			windowEnd = (std::min)(static_cast<int>(sourceLine.size()), windowEnd + 4);
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
