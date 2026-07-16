#pragma once

#include <iosfwd>
#include <string>

namespace shard
{
    class ObjectInstance;
    class SymbolTable;
}

namespace shard::exceptions
{
    /// <summary>
    /// Pretty-prints an unhandled runtime exception: a "Unhandled exception.
    /// &lt;Type&gt;: &lt;message&gt;" banner followed by a resolved stack trace,
    /// annotating each frame with its declaring source file and line when the
    /// method can be located in <paramref name="table"/>.
    /// </summary>
    void PrintUnhandled(std::wostream& out, ObjectInstance* exception,
                        const std::wstring& message, const std::wstring& stackTrace,
                        SymbolTable* table);

    /// <summary>
    /// Prints a "Critical error:" banner for a fatal, non-recoverable failure.
    /// <paramref name="message"/> is a narrow (UTF-8) string, typically the
    /// <c>what()</c> of a caught <c>std::exception</c>.
    /// </summary>
    void PrintCritical(std::wostream& out, const char* message);
}
