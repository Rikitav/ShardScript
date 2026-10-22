#pragma once
#include <shard/Definitions.hpp>
#include <shard/ApplicationDomain.hpp>

#include <shard/compilation/VirtualProgramImage.hpp>

#include <shard/lexical/SourceProvider.hpp>

#include <shard/parsing/Diagnostics.hpp>
#include <shard/parsing/SyntaxTree.hpp>

#include <shard/pragmatic/PragmaticModel.hpp>

#include <shard/semantic/SemanticModel.hpp>

#include <gmt/Containers.hpp>
#include <gmt/SharedLibrary.hpp>

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace shard
{
	// Owns the whole compilation pipeline state (tree, diagnostics, models,
	// program image, loaded native libraries) and drives the phases in order.
	// The context must outlive any ApplicationDomain produced by compile(),
	// since the domain borrows the semantic and pragmatic models.
	// Native libraries must be loaded before analyze_tree(), so their symbols
	// participate in analysis.
	class SHARD_API CompilationContext
	{
		// libraries must unload only after every arena is destroyed: symbols a
		// library emplaces register destructor thunks instantiated inside the
		// library binary, so the library code must stay loaded while the
		// semantic model tears down
		gmt::vector<gmt::SharedLibrary> m_libraries;

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
		void parse_source(const gmt::wstring& name, const gmt::wstring& source);

		void add_lib(const std::filesystem::path& path);
		void add_lib(LibraryHandle handle);
		void add_lib(gmt::SharedLibrary library);
		void add_libraries(const gmt::vector<std::filesystem::path>& paths);

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
