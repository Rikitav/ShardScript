#pragma once
#include <string>

namespace shard::parsing::analysis
{
	enum class DiagnosticSeverity
	{
		Info,
		Warning,
		Error
	};

	static inline std::wstring severity_to_wstring(const DiagnosticSeverity& severity)
	{
		switch (severity)
		{
			case DiagnosticSeverity::Info:
				return L"Info";

			case DiagnosticSeverity::Warning:
				return L"Warn";

			case DiagnosticSeverity::Error:
				return L"Error";

			default:
				return L"";
		}
	}
}