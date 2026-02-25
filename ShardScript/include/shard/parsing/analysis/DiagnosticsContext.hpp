#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/syntax/SyntaxToken.hpp>
#include <shard/parsing/analysis/Diagnostic.hpp>

#include <string>
#include <vector>
#include <ostream>
#include <stdexcept>

namespace shard
{
	class SHARD_API diagnostics_exception : public std::runtime_error
	{
	public:
		diagnostics_exception(const char* message);
		diagnostics_exception(std::string& message);
	};

	class SHARD_API DiagnosticsContext
	{
	public:
		bool AnyError = false;
		std::vector<Diagnostic> Diagnostics;

		inline DiagnosticsContext() : Diagnostics() {}

		void ReportError(shard::SyntaxToken token, std::wstring message);
		void ReportWarning(shard::SyntaxToken token, std::wstring message);
		void ReportInfo(shard::SyntaxToken token, std::wstring message);
		void WriteDiagnostics(std::wostream& out);
		void Reset();
	};
}