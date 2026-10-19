#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/Diagnostics.hpp>
#include <shard/parsing/SyntaxTree.hpp>

#include <shard/pragmatic/PragmaticModel.hpp>

#include <shard/semantic/SemanticModel.hpp>

#include <filesystem>
#include <memory>
#include <string>

namespace shard
{
	class ApplicationDomain;
	class VirtualProgramImage;
	class SourceProvider;

	// Owns the whole compilation pipeline state (tree, diagnostics, models,
	// program image) and drives the phases in order. The context must outlive
	// any ApplicationDomain produced by compile(), since the domain borrows
	// the semantic and pragmatic models.
	// TODO: restore dynamic library loading (AddLib/AddLibraries with ShardLib
	// entry points and dependency resolution) once a loader API lands.
	class SHARD_API CompilationContext
	{
		SyntaxTree m_tree;
		DiagnosticsContext m_diagnostics;
		SemanticModel m_model;
		PragmaticModel m_pragmatic;
		std::unique_ptr<VirtualProgramImage> m_image;

	public:
		CompilationContext();
		~CompilationContext();

		CompilationContext(const CompilationContext&) = delete;
		CompilationContext& operator=(const CompilationContext&) = delete;

		void enrich_tree(SourceProvider& provider);
		void parse_file(const std::filesystem::path path);
		void parse_source(const std::wstring& name, const std::wstring& source);

		bool analyze_tree();
		bool emit_program();
		std::unique_ptr<ApplicationDomain> compile();

		SyntaxTree& get_syntax_tree();
		SemanticModel& get_semantic_model();
		PragmaticModel& get_pragmatic_model();
		DiagnosticsContext& get_diagnostics();
		VirtualProgramImage& get_program();
	};
}
