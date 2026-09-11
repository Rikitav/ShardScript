#pragma once
#include <shard/Definitions.hpp>

#include <shard/analysis/TextLocation.hpp>
#include <shard/analysis/DiagnosticSeverity.hpp>
#include <shard/parsing/SyntaxToken.hpp>

#include <string>

namespace shard
{
	namespace shard
	{
		enum class DiagnosticSeverity
		{
			Info,
			Warning,
			Error
		};
	}

	class SHARD_API Diagnostic
	{
	public:
		const DiagnosticSeverity Severity;
		const std::wstring Description;
		const SyntaxToken Token;
		const TextLocation Location;

		Diagnostic(SyntaxToken token, DiagnosticSeverity severity, std::wstring description);
		Diagnostic(TextLocation token, DiagnosticSeverity severity, std::wstring description);
	};

	class SHARD_API DiagnosticsContext
	{
	public:
		bool AnyError = false;
		std::vector<Diagnostic> Diagnostics;

		DiagnosticsContext() = default;

		void ReportError(SyntaxToken token, const std::wstring& message);
		void ReportError(SyntaxToken token, const wchar_t* message);
		void ReportWarning(SyntaxToken token, const std::wstring& message);
		void ReportWarning(SyntaxToken token, const wchar_t* message);
		void ReportInfo(SyntaxToken token, const std::wstring& message);
		void ReportInfo(SyntaxToken token, const wchar_t* message);
		void WriteDiagnostics(std::wostream& out);
		void Reset();
	};

	SHARD_API std::wstring severity_to_wstring(const DiagnosticSeverity& severity);
}
