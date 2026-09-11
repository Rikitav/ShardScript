#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/TextLocation.hpp>
#include <shard/parsing/SyntaxToken.hpp>

#include <vector>
#include <string>
#include <span>

namespace shard
{
	enum class DiagnosticSeverity
	{
		Info,
		Warning,
		Error
	};

	struct SHARD_API Diagnostic
	{
	private:
		DiagnosticSeverity m_severity;
		std::wstring m_description;
		SyntaxToken m_blameToken;
		TextLocation m_location;

	public:
		Diagnostic(const SyntaxToken& token, DiagnosticSeverity severity, const std::wstring& description);
		Diagnostic(TextLocation location, DiagnosticSeverity severity, const std::wstring& description);
		Diagnostic(TextLocation location, const SyntaxToken& token, DiagnosticSeverity severity, const std::wstring& description);

		inline DiagnosticSeverity get_severity() const	{ return m_severity; }
		inline string_t get_description() const			{ return m_description.c_str(); }
		inline SyntaxToken get_blame_token() const		{ return m_blameToken; }
		inline TextLocation get_location() const		{ return m_location; }
	};

	class SHARD_API DiagnosticsContext
	{
		std::vector<Diagnostic> m_diagnostics;
		bool m_anyError;

	public:
		DiagnosticsContext() = default;

		void report_error(const SyntaxToken& token, const std::wstring& description);
		void report_error(const SyntaxToken& token, string_t description);
		void report_error(const TextLocation& location, const std::wstring& description);
		void report_error(const TextLocation& location, string_t description);

		void report_warning(const SyntaxToken& token, const std::wstring& description);
		void report_warning(const SyntaxToken& token, string_t description);
		void report_warning(const TextLocation& location, const std::wstring& description);
		void report_warning(const TextLocation& location, string_t description);
		
		void report_info(const SyntaxToken& token, const std::wstring& description);
		void report_info(const SyntaxToken& token, string_t description);
		void report_info(const TextLocation& location, const std::wstring& description);
		void report_info(const TextLocation& location, string_t description);
		
		std::span<const Diagnostic> get_diagnostics() const;

		void write_diagnostics(std::wostream& out) const;
		void reset();
	};

	SHARD_API string_t severity_to_wstring(const DiagnosticSeverity severity);
}
