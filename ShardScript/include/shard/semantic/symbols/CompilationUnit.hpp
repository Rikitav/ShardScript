#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/semantic/SyntaxSymbol.hpp>

#include <gmt/Arena.hpp>
#include <gmt/Ref.hpp>

namespace shard
{
	class SHARD_API CompilationUnit final : public SyntaxSymbol
	{
	public:
		CompilationUnit();
		virtual ~CompilationUnit() = default;

		CompilationUnit(const CompilationUnit& other) = delete;
		CompilationUnit& operator=(const CompilationUnit& other) = delete;

		CompilationUnit(CompilationUnit&& other) = delete;
		CompilationUnit& operator=(CompilationUnit&& other) = delete;
	};
}
