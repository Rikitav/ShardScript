#pragma once
#include <shard/Definitions.hpp>
#include <gmt/Containers.hpp>
#include <gmt/SharedLibrary.hpp>
#include <shard/parsing/SyntaxTree.hpp>
#include <shard/parsing/Diagnostics.hpp>
#include <shard/semantic/SemanticModel.hpp>
#include <shard/pragmatic/PragmaticModel.hpp>

namespace shard
{
	class CompilationContext;
	class ApplicationDomain;

	class SHARD_API CompilationState final
	{
		friend class CompilationContext;
		friend class ApplicationDomain;

		gmt::vector<gmt::SharedLibrary> m_libraries;
		SyntaxTree m_tree;
		DiagnosticsContext m_diagnostics;
		SemanticModel m_model;
		PragmaticModel m_pragmatic;

	public:
		CompilationState();
		~CompilationState();

		CompilationState(const CompilationState&) = delete;
		CompilationState& operator=(const CompilationState&) = delete;
	};
}
