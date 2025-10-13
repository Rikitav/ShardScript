#pragma once
#include <string>

using namespace std;

namespace shard::syntax::analysis
{
	enum class DiagnosticSeverity
	{
		Info,
		Warning,
		Error
	};

	static string toString(const DiagnosticSeverity& severity)
	{
		switch (severity)
		{
			case DiagnosticSeverity::Info:
				return "Info";

			case DiagnosticSeverity::Warning:
				return "Warn";

			case DiagnosticSeverity::Error:
				return "Error";

			default:
				return "";
		}
	}
}