#pragma once
#include <string>
#include <shard/syntax/SyntaxToken.h>
#include <shard/parsing/analysis/TextLocation.h>
#include <shard/parsing/analysis/DiagnosticSeverity.h>

namespace shard::parsing::analysis
{
	class Diagnostic
	{
	public:
		const DiagnosticSeverity Severity;
		const std::string Description;
		const shard::syntax::SyntaxToken Token;
		const TextLocation Location;

		Diagnostic(shard::syntax::SyntaxToken token, DiagnosticSeverity severity, std::string description);
	};
}
