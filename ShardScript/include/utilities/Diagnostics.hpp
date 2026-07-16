#pragma once

#include <iosfwd>
#include <string>

#include <shard/analysis/Diagnostic.hpp>
#include <shard/analysis/DiagnosticSeverity.hpp>

namespace shard
{
    class DiagnosticsContext;
}

namespace shard::diagnostics
{
    /// <summary>
    /// Human-readable severity label ("error", "warning", "info").
    /// </summary>
    const wchar_t* SeverityText(DiagnosticSeverity severity);

    /// <summary>
    /// ANSI color appropriate to the severity (see shard::console).
    /// </summary>
    const wchar_t* SeverityColor(DiagnosticSeverity severity);

    /// <summary>
    /// Reads the 1-based <paramref name="targetLine"/> from a source file.
    /// Returns an empty string if the file cannot be opened or the line is absent.
    /// </summary>
    std::wstring ReadSourceLine(const std::wstring& filePath, int targetLine);

    /// <summary>
    /// Pretty-prints every diagnostic in <paramref name="context"/> to
    /// <paramref name="out"/>: severity, message, a " --> file:line:col" header,
    /// the offending source line, and a caret underline.  Long lines are windowed
    /// around the diagnostic to stay readable.
    /// </summary>
    void Print(std::wostream& out, const DiagnosticsContext& context);

    /// <summary>
    /// Pretty-prints a single diagnostic (see overload above for the format).
    /// </summary>
    void Print(std::wostream& out, const Diagnostic& diagnostic);
}
