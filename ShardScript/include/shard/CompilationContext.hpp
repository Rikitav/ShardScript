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
	class SHARD_API CompilationContext
	{
		gmt::shared_ptr<CompilationState> m_state;
		gmt::vector<gmt::SharedLibrary>& m_libraries;
		SyntaxTree& m_tree;
		DiagnosticsContext& m_diagnostics;
		SemanticModel& m_model;
		PragmaticModel& m_pragmatic;
		gmt::unique_ptr<VirtualProgramImage> m_image;

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
		gmt::unique_ptr<ApplicationDomain> compile();

		SyntaxTree& get_syntax_tree();
		SemanticModel& get_semantic_model();
		PragmaticModel& get_pragmatic_model();
		DiagnosticsContext& get_diagnostics();
		VirtualProgramImage& get_program();
	};
}
