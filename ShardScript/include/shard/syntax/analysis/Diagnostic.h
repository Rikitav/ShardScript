#pragma once
#include <string>
#include <shard/syntax/SyntaxToken.h>
#include <shard/syntax/analysis/TextLocation.h>
#include <shard/syntax/analysis/DiagnosticSeverity.h>

using namespace std;

namespace shard::syntax::analysis
{
	class Diagnostic
	{
	public:
		const DiagnosticSeverity Severity;
		const string Description;
		const SyntaxToken Token;
		const TextLocation Location;

		Diagnostic(SyntaxToken token, DiagnosticSeverity severity, string description);
	};
}
