#pragma once
#include <shard/syntax/analysis/Diagnostic.h>
#include <shard/syntax/SyntaxToken.h>
#include <string>
#include <vector>

namespace shard::syntax::analysis
{
	class DiagnosticsContext
	{
	public:
		bool AnyError = false;
		vector<Diagnostic> Diagnostics;

		DiagnosticsContext() : Diagnostics() {}

		void ReportError(SyntaxToken token, string message);
		void ReportWarning(SyntaxToken token, string message);
		void ReportInfo(SyntaxToken token, string message);
	};
}