#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/parsing/analysis/TextLocation.hpp>
#include <shard/parsing/analysis/DiagnosticSeverity.hpp>
#include <shard/syntax/SyntaxToken.hpp>

#include <string>

namespace shard
{
	class SHARD_API Diagnostic
	{
	public:
		const DiagnosticSeverity Severity;
		const std::wstring Description;
		const shard::SyntaxToken Token;
		const TextLocation Location;

		Diagnostic(shard::SyntaxToken token, DiagnosticSeverity severity, std::wstring description);
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
