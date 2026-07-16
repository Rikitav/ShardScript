#include <utilities/Diagnostics.hpp>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <locale>
#include <string>

#include <utilities/Console.hpp>
#include <shard/analysis/DiagnosticsContext.hpp>

namespace shard::diagnostics
{
    const wchar_t* SeverityText(DiagnosticSeverity severity)
    {
        switch (severity)
        {
            case DiagnosticSeverity::Warning: return L"warning";
            case DiagnosticSeverity::Info:    return L"info";
            default:                          return L"error";
        }
    }

    const wchar_t* SeverityColor(DiagnosticSeverity severity)
    {
        switch (severity)
        {
            case DiagnosticSeverity::Warning: return console::FG_YELLOW;
            case DiagnosticSeverity::Info:    return console::FG_BLUE;
            default:                          return console::FG_RED;
        }
    }

    std::wstring ReadSourceLine(const std::wstring& filePath, int targetLine)
    {
        std::filesystem::path sourcePath(filePath);
        std::wifstream in(sourcePath);
        in.imbue(std::locale::classic());
        if (!in.is_open())
            return L"";

        std::wstring line;
        int currentLine = 1;
        while (currentLine < targetLine && std::getline(in, line))
            ++currentLine;

        if (!std::getline(in, line))
            return L"";

        if (!line.empty() && line.back() == L'\r')
            line.pop_back();

        return line;
    }

    void Print(std::wostream& out, const DiagnosticsContext& context)
    {
        for (const Diagnostic& diag : context.Diagnostics)
            Print(out, diag);
    }

    void Print(std::wostream& out, const Diagnostic& diag)
    {
        const TextLocation& loc = diag.Location;
        const wchar_t* severityColor = SeverityColor(diag.Severity);

        out << severityColor << SeverityText(diag.Severity) << console::ST_RESET
            << L": " << diag.Description << std::endl;

        out << console::FG_LIGHT_GRAY << L" --> " << console::ST_RESET
            << console::FG_CYAN << loc.FileName << console::ST_RESET
            << console::FG_LIGHT_GRAY << L":" << loc.Line << L":" << loc.Offset << console::ST_RESET << std::endl;

        out << console::FG_LIGHT_GRAY << L"    |" << console::ST_RESET << std::endl;

        std::wstring sourceLine = ReadSourceLine(loc.FileName, loc.Line);
        if (!sourceLine.empty())
        {
            constexpr int MaxVisibleLineLength = 160;
            constexpr int CaretWindowRadius = 60;

            int start = (std::max)(0, loc.Offset - 1);
            int length = (std::max)(1, loc.Length);

            if (start > static_cast<int>(sourceLine.length()))
                start = static_cast<int>(sourceLine.length());

            if (start + length > static_cast<int>(sourceLine.length()))
                length = static_cast<int>(sourceLine.length()) - start;

            // If the source line is extremely long, show only a window around the
            // diagnostic so the output stays readable.
            std::wstring displayedLine = sourceLine;
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

            out << console::FG_LIGHT_GRAY << std::setw(3) << loc.Line << L" | " << console::ST_RESET;
            if (trimmedPrefix)
                out << L"...";

            out << displayedLine;
            if (trimmedSuffix)
                out << L"...";

            out << std::endl;

            int caretStart = start - lineOffset;
            if (trimmedPrefix)
                caretStart += 3;

            out << console::FG_LIGHT_GRAY << L"    | " << console::ST_RESET
                << std::wstring(caretStart, L' ')
                << severityColor << std::wstring(length, L'^') << console::ST_RESET << std::endl;
        }
    }
}
