#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/parsing/analysis/TextLocation.h>
#include <shard/parsing/analysis/DiagnosticSeverity.h>
#include <shard/syntax/SyntaxToken.h>

#include <string>

namespace shard::parsing::analysis
{
	class SHARD_API Diagnostic
	{
	public:
		const DiagnosticSeverity Severity;
		const std::wstring Description;
		const shard::syntax::SyntaxToken Token;
		const TextLocation Location;

		Diagnostic(shard::syntax::SyntaxToken token, DiagnosticSeverity severity, std::wstring description);
		Diagnostic(const Diagnostic& other);

		inline Diagnostic& operator=(const Diagnostic& other)
		{
			if (this != &other)
			{
				this->~Diagnostic();
				new (this) Diagnostic(other);
			}

			return *this;
		}
	};
}
