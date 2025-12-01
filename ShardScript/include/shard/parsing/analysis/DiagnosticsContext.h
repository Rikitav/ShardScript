#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/syntax/SyntaxToken.h>
#include <shard/parsing/analysis/Diagnostic.h>

#include <string>
#include <vector>
#include <ostream>

namespace shard::parsing::analysis
{
	class SHARD_API DiagnosticsContext
	{
	public:
		bool AnyError = false;
		std::vector<Diagnostic> Diagnostics;

		inline DiagnosticsContext() : Diagnostics() {}

		void ReportError(shard::syntax::SyntaxToken token, std::wstring message);
		void ReportWarning(shard::syntax::SyntaxToken token, std::wstring message);
		void ReportInfo(shard::syntax::SyntaxToken token, std::wstring message);
		void WriteDiagnostics(std::wostream& out);
		void Reset();
	};
}