#include <shard/parsing/analysis/DiagnosticSeverity.hpp>

using namespace shard;

std::wstring severity_to_wstring(const DiagnosticSeverity& severity)
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
